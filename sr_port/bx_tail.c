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
#include "mdq.h"
#include "mmemory.h"

LITREF octabstruct oc_tab[];

/*	structure of jmps is as follows:

	sense		OC_AND		OC_OR

	TRUE		op1		op1
			jmpf next	jmpt addr
			op2		op2
			jmpt addr	jmpt addr

	FALSE		op1		op1
			jmpf addr	jmpt next
			op2		op2
			jmpf addr	jmpf addr
*/

void bx_tail(triple *t, bool sense, oprtype *addr)
/*
triple *t;              triple to be processed
bool sense;             code to be generated is jmpt or jmpf
oprtype *addr;          address to jmp
*/
{
	triple *ref;
	oprtype *p;

	assert(sense == TRUE || sense == FALSE);
	assert(oc_tab[t->opcode].octype & OCT_BOOL);
	assert(t->operand[0].oprclass == TRIP_REF);
	assert(t->operand[1].oprclass == TRIP_REF || t->operand[1].oprclass == 0);
	switch (t->opcode)
	{
	case OC_COBOOL:
		ex_tail(&t->operand[0]);
		if (t->operand[0].oprval.tref->opcode == OC_GETTRUTH)
		{
			dqdel(t->operand[0].oprval.tref, exorder);
			t->opcode = sense ? OC_JMPTSET : OC_JMPTCLR;
			t->operand[0] = put_indr(addr);
			return;
		}
		ref = maketriple(sense ? OC_JMPNEQ : OC_JMPEQU);
		ref->operand[0] = put_indr(addr);
		dqins(t, exorder, ref);
		return;
	case OC_COM:
		bx_tail(t->operand[0].oprval.tref , !sense, addr);
		t->opcode = OC_NOOP;
		t->operand[0].oprclass = 0;
		return;
	case OC_NEQU:
		sense = ! sense;
		/* caution: fall through */
	case OC_EQU:
		bx_relop(t, OC_EQU, sense ? OC_JMPNEQ : OC_JMPEQU, addr);
		break;
	case OC_NPATTERN:
		sense = ! sense;
		/* caution: fall through */
	case OC_PATTERN:
		bx_relop(t, OC_PATTERN, sense ? OC_JMPNEQ : OC_JMPEQU, addr);
		break;
	case OC_NFOLLOW:
		sense = ! sense;
		/* caution: fall through */
	case OC_FOLLOW:
		bx_relop(t, OC_FOLLOW, sense ? OC_JMPGTR : OC_JMPLEQ, addr);
		break;
	case OC_NSORTS_AFTER:
		sense = ! sense;
		/* caution: fall through */
	case OC_SORTS_AFTER:
		bx_relop(t, OC_SORTS_AFTER, sense ? OC_JMPGTR : OC_JMPLEQ, addr);
		break;
	case OC_NCONTAIN:
		sense = ! sense;
		/* caution: fall through */
	case OC_CONTAIN:
		bx_relop(t, OC_CONTAIN, sense ? OC_JMPEQU : OC_JMPNEQ, addr);
		break;
	case OC_NGT:
		sense = ! sense;
		/* caution: fall through */
	case OC_GT:
		bx_relop(t, OC_NUMCMP, sense ? OC_JMPGTR : OC_JMPLEQ, addr);
		break;
	case OC_NLT:
		sense = ! sense;
		/* caution: fall through */
	case OC_LT:
		bx_relop(t, OC_NUMCMP, sense ? OC_JMPLSS : OC_JMPGEQ, addr);
		break;
	case OC_NAND:
		sense = ! sense;
		/* caution: fall through */
	case OC_AND:
		bx_boolop(t, FALSE, sense, sense, addr);
		return;
	case OC_NOR:
		sense = !sense;
		/* caution: fall through */
	case OC_OR:
		bx_boolop(t, TRUE, !sense, sense, addr);
		return;
	default:
		GTMASSERT;
	}
	for (p = t->operand ; p < &(t->operand[2]) ; p++)
		if (p->oprclass == TRIP_REF)
			ex_tail(p);
	return;
}
