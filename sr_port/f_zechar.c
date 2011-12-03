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

#include "gtm_string.h"

#include "compiler.h"
#include "opcode.h"
#include "toktyp.h"
#include "stringpool.h"
#include "gtm_iconv.h"
#include "io.h"
#include "iosp.h"
#ifdef __MVS__
#include <unistd.h>
#endif
#include "advancewindow.h"
#include "ebc_xlat.h"

GBLREF char window_token;
GBLREF spdesc stringpool;

int f_zechar( oprtype *a, opctype op )
{
	triple *root, *last, *curr;
	oprtype argv[CHARMAXARGS], *argp;
	mval v;
	bool all_lits;
	char *c;
	int argc, i;
	error_def(ERR_FCHARMAXARGS);
	error_def(ERR_TEXT);

	all_lits = TRUE;
	argp = &argv[0];
	argc = 0;
	for (;;)
	{
		if (!intexpr(argp))
			return FALSE;
		assert(argp->oprclass == TRIP_REF);
		if (argp->oprval.tref->opcode != OC_ILIT)
			all_lits = FALSE;
		argc++;
		argp++;
		if (window_token != TK_COMMA)
			break;
		advancewindow();
		if (argc >= CHARMAXARGS)
		{	stx_error(ERR_FCHARMAXARGS);
			return FALSE;
		}
	}
	if (all_lits)
	{
		iconv_t		tmp_cvt_cd;
		unsigned char	*tmp_ptr;
		unsigned int	tmp_len;

		if (stringpool.top - stringpool.free < argc)
			stp_gcol(argc);
		v.mvtype = MV_STR;
		v.str.addr = c = (char *) stringpool.free;
		argp = &argv[0];
		for (; argc > 0 ;argc--, argp++)
		{
			i = argp->oprval.tref->operand[0].oprval.ilit;
			if ((i >= 0) && (i < 256))	/* only true for single byte character set */
				*c++ = i;
		}
		*c = '\0';
		v.str.len = c - v.str.addr;
		tmp_ptr = (unsigned char *)v.str.addr;
		tmp_len = v.str.len;
		ICONV_OPEN_CD(tmp_cvt_cd, "IBM-1047", "ISO8859-1");
		ICONVERT(tmp_cvt_cd, &tmp_ptr, &tmp_len, &tmp_ptr, &tmp_len);
		ICONV_CLOSE_CD(tmp_cvt_cd);
		stringpool.free =(unsigned char *)  c;
		s2n(&v);
		*a = put_lit(&v);
		return TRUE;
	}
	root = maketriple(op);
	root->operand[0] = put_ilit(argc + 2);
	curr = newtriple(OC_PARAMETER);
	curr->operand[0] = put_ilit(1);		/*	1: need ebcdic/ascii convert	*/
	root->operand[1] = put_tref(curr);
	last = curr;
	argp = &argv[0];
	for (; argc > 0 ;argc--, argp++)
	{
		curr = newtriple(OC_PARAMETER);
		curr->operand[0] = *argp;
		last->operand[1] = put_tref(curr);
		last = curr;
	}
	ins_triple(root);
	*a = put_tref(root);
	return TRUE;
}
