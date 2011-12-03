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
#include "opcode.h"
#include "toktyp.h"
#include "advancewindow.h"
#include "stringpool.h"
#include "rwformat.h"

GBLREF char window_token;
GBLREF mident window_ident;

int rwformat(void)
{
	triple *ref, *argcnt, *parm;
	oprtype x;
	error_def(ERR_RWFORMAT);
	error_def(ERR_COMMAORRPARENEXP);
	error_def(ERR_CTLMNEXPECTED);
	int n;
	char *ptr;
	mval key;

	ref = 0;
	for (;;)
	{
		switch (window_token)
		{
		case TK_EXCLAIMATION:
			n = 0;
			do
			{
				n++;
				advancewindow();
			} while (window_token == TK_EXCLAIMATION);
			ref = maketriple(OC_WTEOL);
			ref->operand[0] = put_ilit(n);
			ins_triple(ref);
			break;
		case TK_HASH:
			advancewindow();
			ref = newtriple(OC_WTFF);
			break;
		case TK_QUESTION:
			advancewindow();
			if (!intexpr(&x))
				return FALSE;
			ref = newtriple(OC_WTTAB);
			ref->operand[0] = x;
			return TRUE;
		case TK_SLASH:
			advancewindow();
			if (window_token != TK_IDENT)
			{
				stx_error(ERR_CTLMNEXPECTED);
				return FALSE;
			}
			for (ptr = window_ident.c , n = 0; *ptr++ && n < sizeof(window_ident) ; n++)
			    ;
			assert(n > 0 && n <= sizeof(window_ident));
			key.mvtype = MV_STR;
			key.str.len = n;
			key.str.addr = window_ident.c;
			s2n(&key);
			s2pool(&(key.str));
			argcnt = parm = newtriple(OC_PARAMETER);
			parm->operand[0] = put_lit(&key);
			advancewindow();
			n = 1;
			if (window_token == TK_LPAREN)
			{
				advancewindow();
				for ( ;; )
				{
					if (!expr(&x))
						return FALSE;
					n++;
					ref = newtriple(OC_PARAMETER);
					ref->operand[0] = x;
					parm->operand[1] = put_tref(ref);
					parm = ref;
					if (window_token == TK_RPAREN)
					{
						advancewindow();
						break;
					}
					if (window_token != TK_COMMA)
					{
						stx_error(ERR_COMMAORRPARENEXP);
						return FALSE;
	 				}
					advancewindow();
				}
			}
			ref = newtriple(OC_IOCONTROL);
			ref->operand[0] = put_ilit(n);
			ref->operand[1] = put_tref(argcnt);
			return TRUE;
		default:
			if (ref)
				return TRUE;
			stx_error(ERR_RWFORMAT);
			return FALSE;
		}
	}
}
