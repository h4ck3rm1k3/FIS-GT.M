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
#include "compiler.h"
#include "mdq.h"
#include "opcode.h"
#include "indir_enum.h"
#include "toktyp.h"
#include "mmemory.h"
#include "advancewindow.h"
#include "cmd.h"

GBLREF char window_token;
GBLREF triple *expr_start, *expr_start_orig;

int m_xecute(void)
{
	triple tmpchain, *oldchain, *obp, *ref0, *ref1, *triptr;
	oprtype *cr, x;

	dqinit(&tmpchain,exorder);
	oldchain = setcurtchain(&tmpchain);
	switch (strexpr(&x))
	{
	case EXPR_FAIL:
		setcurtchain(oldchain);
		return FALSE;
	case EXPR_INDR:
		if (window_token != TK_COLON)
		{
			make_commarg(&x,indir_xecute);
			break;
		}
		/* caution: fall through */
	case EXPR_GOOD:
		ref0 = maketriple(OC_COMMARG);
		ref0->operand[0] = x;
		ref0->operand[1] = put_ilit(indir_linetail);
		ins_triple(ref0);
	}
	setcurtchain(oldchain);
	if (window_token == TK_COLON)
	{
		advancewindow();
		cr = (oprtype *) mcalloc(sizeof(oprtype));
		if (!bool_expr((bool) FALSE,cr))
			return FALSE;
		if (expr_start != expr_start_orig)
		{
			triptr = newtriple(OC_GVRECTARG);
			triptr->operand[0] = put_tref(expr_start);
		}
		obp = oldchain->exorder.bl;
		dqadd(obp,&tmpchain,exorder);		/* violates info hiding */
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
	dqadd(obp,&tmpchain,exorder);		/* violates info hiding */
	return TRUE;
}
