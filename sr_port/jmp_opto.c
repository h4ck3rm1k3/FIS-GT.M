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
#include "mdq.h"
#include "jmp_opto.h"

#define JOPT_NO_OPT 1
#define JOPT_REP_JMP 2
#define JOPT_REF_NXT_TRP 3

#define PTR_NOT_DEFINED 0
#define IND_NOT_DEFINED ((unsigned char)-2)
#define NO_ENTRY ((unsigned char)-1)

#define NUM_JO_TBL_ELE 11
typedef struct
{	unsigned int	opcode;
	unsigned int	index;
	unsigned int	opto_flag[NUM_JO_TBL_ELE];
}jump_opto_struct;

LITREF octabstruct	oc_tab[];	/* op-code table */
GBLREF triple		t_orig;		/* head of triples */

const static readonly jump_opto_struct jump_opto_table[NUM_JO_TBL_ELE] =
{
	{	OC_JMP,		/* opcode */
		0,		/* index */
		{
			 JOPT_REP_JMP,	/* OC_JMP */		 JOPT_NO_OPT,	/* OC_JMPTSET */
			 JOPT_NO_OPT,	/* OC_JMPTCLR */	 JOPT_NO_OPT,	/* OC_JMPEQU */
			 JOPT_NO_OPT,	/* OC_JMPNEQ */		 JOPT_NO_OPT,	/* OC_JMPGTR */
			 JOPT_NO_OPT,	/* OC_JMPLEQ */		 JOPT_NO_OPT,	/* OC_JMPLSS */
			 JOPT_NO_OPT,	/* OC_JMPGEQ */		 JOPT_NO_OPT,	/* HALT */
			 JOPT_NO_OPT	/* RET */
		}
	},
	{	OC_JMPTSET,	/* opcode */
		1,		/* index */
		{
			 JOPT_REP_JMP,	/* OC_JMP */		 JOPT_REP_JMP,	/* OC_JMPTSET */
			 JOPT_REF_NXT_TRP, /* OC_JMPTCLR */	 JOPT_NO_OPT,	/* OC_JMPEQU */
			 JOPT_NO_OPT,	/* OC_JMPNEQ */		 JOPT_NO_OPT,	/* OC_JMPGTR */
			 JOPT_NO_OPT,	/* OC_JMPLEQ */		 JOPT_NO_OPT,	/* OC_JMPLSS */
			 JOPT_NO_OPT,	/* OC_JMPGEQ */		 JOPT_NO_OPT,	/* HALT */
			 JOPT_NO_OPT	/* RET */
		}
	},
	{	OC_JMPTCLR,	/* opcode */
		2,		/* index */
		{
			 JOPT_REP_JMP,	/* OC_JMP */		 JOPT_REF_NXT_TRP,	/* OC_JMPTSET */
			 JOPT_REP_JMP,	/* OC_JMPTCLR */	 JOPT_NO_OPT,	/* OC_JMPEQU */
			 JOPT_NO_OPT,	/* OC_JMPNEQ */		 JOPT_NO_OPT,	/* OC_JMPGTR */
			 JOPT_NO_OPT,	/* OC_JMPLEQ */		 JOPT_NO_OPT,	/* OC_JMPLSS */
			 JOPT_NO_OPT,	/* OC_JMPGEQ */		 JOPT_NO_OPT,	/* HALT */
			 JOPT_NO_OPT	/* RET */
		}
	},
	{	OC_JMPEQU,	/* opcode */
		3,		/* index */
		{
			 JOPT_REP_JMP,	/* OC_JMP */		 JOPT_NO_OPT,	/* OC_JMPTSET */
			 JOPT_NO_OPT,	/* OC_JMPTCLR */	 JOPT_REP_JMP,	/* OC_JMPEQU */
			 JOPT_REF_NXT_TRP, /* OC_JMPNEQ */	 JOPT_REF_NXT_TRP,	/* OC_JMPGTR */
			 JOPT_NO_OPT,	/* OC_JMPLEQ */		 JOPT_REF_NXT_TRP,	/* OC_JMPLSS */
			 JOPT_NO_OPT,	/* OC_JMPGEQ */		 JOPT_NO_OPT,	/* HALT */
			 JOPT_NO_OPT	/* RET */
		}
	},
	{	OC_JMPNEQ,	/* opcode */
		4,		/* index */
		{
			 JOPT_REP_JMP,	/* OC_JMP */		 JOPT_NO_OPT,	/* OC_JMPTSET */
			 JOPT_NO_OPT,	/* OC_JMPTCLR */	 JOPT_REF_NXT_TRP,	/* OC_JMPEQU */
			 JOPT_REP_JMP,	/* OC_JMPNEQ */		 JOPT_NO_OPT,	/* OC_JMPGTR */
			 JOPT_NO_OPT,	/* OC_JMPLEQ */		 JOPT_NO_OPT,	/* OC_JMPLSS */
			 JOPT_NO_OPT,	/* OC_JMPGEQ */		 JOPT_NO_OPT,	/* HALT */
			 JOPT_NO_OPT	/* RET */
		}
	},
	{	OC_JMPGTR,	/* opcode */
		5,		/* index */
		{
			 JOPT_REP_JMP,	/* OC_JMP */		 JOPT_NO_OPT,		/* OC_JMPTSET */
			 JOPT_NO_OPT,	/* OC_JMPTCLR */	 JOPT_REF_NXT_TRP,	/* OC_JMPEQU */
			 JOPT_REP_JMP,	/* OC_JMPNEQ */		 JOPT_REP_JMP,		/* OC_JMPGTR */
			 JOPT_REF_NXT_TRP, /* OC_JMPLEQ */	 JOPT_REF_NXT_TRP,	/* OC_JMPLSS */
			 JOPT_NO_OPT,	/* OC_JMPGEQ */		 JOPT_NO_OPT,		/* HALT */
			 JOPT_NO_OPT	/* RET */
		}
	},
	{	OC_JMPLEQ,	/* opcode */
		6,		/* index */
		{
			 JOPT_REP_JMP,	/* OC_JMP */		 JOPT_NO_OPT,	/* OC_JMPTSET */
			 JOPT_NO_OPT,	/* OC_JMPTCLR */	 JOPT_NO_OPT,	/* OC_JMPEQU */
			 JOPT_NO_OPT,	/* OC_JMPNEQ */		 JOPT_REF_NXT_TRP,	/* OC_JMPGTR */
			 JOPT_REP_JMP,	/* OC_JMPLEQ */		 JOPT_NO_OPT,	/* OC_JMPLSS */
			 JOPT_NO_OPT,	/* OC_JMPGEQ */		 JOPT_NO_OPT,	/* HALT */
			 JOPT_NO_OPT	/* RET */
		}
	},
	{	OC_JMPLSS,	/* opcode */
		7,		/* index */
		{
			 JOPT_REP_JMP,	/* OC_JMP */		 JOPT_NO_OPT,	/* OC_JMPTSET */
			 JOPT_NO_OPT,	/* OC_JMPTCLR */	 JOPT_REF_NXT_TRP,	/* OC_JMPEQU */
			 JOPT_REP_JMP,	/* OC_JMPNEQ */		 JOPT_REF_NXT_TRP,	/* OC_JMPGTR */
			 JOPT_REP_JMP,	/* OC_JMPLEQ */		 JOPT_REP_JMP,	/* OC_JMPLSS */
			 JOPT_REF_NXT_TRP,	/* OC_JMPGEQ */		 JOPT_NO_OPT,	/* HALT */
			 JOPT_NO_OPT	/* RET */
		}
	},
	{	OC_JMPGEQ,	/* opcode */
		8,		/* index */
		{
			 JOPT_REP_JMP,	/* OC_JMP */		 JOPT_NO_OPT,	/* OC_JMPTSET */
			 JOPT_NO_OPT,	/* OC_JMPTCLR */	 JOPT_NO_OPT,	/* OC_JMPEQU */
			 JOPT_NO_OPT,	/* OC_JMPNEQ */		 JOPT_NO_OPT,	/* OC_JMPGTR */
			 JOPT_NO_OPT,	/* OC_JMPLEQ */		 JOPT_REF_NXT_TRP,	/* OC_JMPLSS */
			 JOPT_REP_JMP,	/* OC_JMPGEQ */		 JOPT_NO_OPT,	/* HALT */
			 JOPT_NO_OPT	/* RET */
		}
	},
	{	OC_HALT,	/* opcode */
		9,		/* index */
		{
			 JOPT_NO_OPT,	/* OC_JMP */		 JOPT_NO_OPT,	/* OC_JMPTSET */
			 JOPT_NO_OPT,	/* OC_JMPTCLR */	 JOPT_NO_OPT,	/* OC_JMPEQU */
			 JOPT_NO_OPT,	/* OC_JMPNEQ */		 JOPT_NO_OPT,	/* OC_JMPGTR */
			 JOPT_NO_OPT,	/* OC_JMPLEQ */		 JOPT_NO_OPT,	/* OC_JMPLSS */
			 JOPT_NO_OPT,	/* OC_JMPGEQ */		 JOPT_NO_OPT,	/* HALT */
			 JOPT_NO_OPT	/* RET */
		}
	},
	{	OC_RET,		/* opcode */
		10,		/* index */
		{
			 JOPT_NO_OPT,	/* OC_JMP */		 JOPT_NO_OPT,	/* OC_JMPTSET */
			 JOPT_NO_OPT,	/* OC_JMPTCLR */	 JOPT_NO_OPT,	/* OC_JMPEQU */
			 JOPT_NO_OPT,	/* OC_JMPNEQ */		 JOPT_NO_OPT,	/* OC_JMPGTR */
			 JOPT_NO_OPT,	/* OC_JMPLEQ */		 JOPT_NO_OPT,	/* OC_JMPLSS */
			 JOPT_NO_OPT,	/* OC_JMPGEQ */		 JOPT_NO_OPT,	/* HALT */
			 JOPT_NO_OPT	/* RET */
		}
	}
};
static	unsigned int *jo_ptr_ray[OPCODE_COUNT];
static	unsigned int jo_ind_ray[OPCODE_COUNT];

static void jo_get_ptrs(unsigned int op)
{
	const jump_opto_struct	*j, *j_top;

	for (j = &jump_opto_table[0], j_top = j + NUM_JO_TBL_ELE; j < j_top; j++)
	{
		if (j->opcode == op)
		{
			jo_ind_ray[op] = j->index;
			jo_ptr_ray[op] = (unsigned int *)&j->opto_flag[0];
			return;
		}
	}
	jo_ind_ray[op] = NO_ENTRY;
	jo_ptr_ray[op] = (unsigned int *)NO_ENTRY;
}

const static readonly oprtype null_operand;

/************************************************************************************************************
 NOTE:	We may which to modify the lookup method at some point in the future.  B. Shear suggests nested switch
	statements. Another option is to do the lookup each time.
***********************************************************************************************************/

void jmp_opto(void)
{
	unsigned int	i, *p, **clrp1, **clrtop1, *clrp2, *clrtop2;
	tbp 		*b;
	triple		*cur_trip, *terminal_trip, *ref_trip, *jump_trip, *next_trip;
	void		get_jo_ptrs();

	for (clrp1 = &jo_ptr_ray[0], clrtop1 = clrp1 + OPCODE_COUNT; clrp1 < clrtop1; clrp1++)
		*clrp1 = (unsigned int *)NO_ENTRY;
	for (clrp2 = &jo_ind_ray[0], clrtop2 = clrp2 + OPCODE_COUNT; clrp2 < clrtop2; clrp2++)
		*clrp2 = NO_ENTRY;

	dqloop(&t_orig, exorder, cur_trip)
	{
		if (cur_trip->opcode == OC_GVSAVTARG)
		{
			for (next_trip = cur_trip->exorder.fl
				     ; oc_tab[next_trip->opcode].octype & OCT_CGSKIP
				     ; next_trip = next_trip->exorder.fl)
				;

			if (next_trip->opcode == OC_GVRECTARG
			    && next_trip->operand[0].oprval.tref == cur_trip
			    && next_trip->jmplist.que.fl == &(next_trip->jmplist))
			{
				next_trip->opcode = OC_NOOP;
				next_trip->operand[0].oprclass = next_trip->operand[1].oprclass = 0;
			}
			continue;
		}
		if (oc_tab[cur_trip->opcode].octype & OCT_JUMP &&
		    cur_trip->opcode != OC_CALL && cur_trip->opcode != OC_CALLSP)
		{
			assert(cur_trip->opcode < OPCODE_COUNT);
			if ((p = jo_ptr_ray[cur_trip->opcode]) == PTR_NOT_DEFINED)
			{
				jo_get_ptrs(cur_trip->opcode);
				p = jo_ptr_ray[cur_trip->opcode];
			}
			assert(cur_trip->operand[0].oprclass == TJMP_REF);
			jump_trip = cur_trip->operand[0].oprval.tref;
			if ((i = jo_ind_ray[jump_trip->opcode]) == IND_NOT_DEFINED)
			{
				jo_get_ptrs(jump_trip->opcode);
				i = jo_ind_ray[jump_trip->opcode];
			}

			while (i != IND_NOT_DEFINED && i != NO_ENTRY)
			{
				switch(p[i])
				{
					case JOPT_NO_OPT:
						i = NO_ENTRY;
						break;
					case JOPT_REF_NXT_TRP:
						if (cur_trip->src.line == jump_trip->src.line)
						{
							dqloop(&jump_trip->jmplist, que, b)
							{
								if (b->bpt = cur_trip)
								{
									dqdel(b, que);
									break;
								}
							}
							dqins(&jump_trip->exorder.fl->jmplist, que, b);

							cur_trip->operand[0].oprval.tref = jump_trip->exorder.fl;
							jump_trip = cur_trip->operand[0].oprval.tref;
							if ((i = jo_ind_ray[jump_trip->opcode]) == IND_NOT_DEFINED)
							{
								jo_get_ptrs(jump_trip->opcode);
								i = jo_ind_ray[jump_trip->opcode];
							}
						} else
							i = NO_ENTRY;
						break;
					case JOPT_REP_JMP:
						if (cur_trip->src.line == jump_trip->src.line)
						{
							assert(jump_trip->operand[0].oprclass == TJMP_REF);
							dqloop(&jump_trip->jmplist, que, b)
							{
								if (b->bpt = cur_trip)
								{
									dqdel(b, que);
									break;
								}
							}
							dqins(&jump_trip->operand[0].oprval.tref->jmplist, que, b);

							cur_trip->operand[0] = jump_trip->operand[0];
							jump_trip = cur_trip->operand[0].oprval.tref;
							if ((i = jo_ind_ray[jump_trip->opcode]) == IND_NOT_DEFINED)
							{
								jo_get_ptrs(jump_trip->opcode);
								i = jo_ind_ray[jump_trip->opcode];
							}
						} else
							i = NO_ENTRY;
						break;
					default:
						GTMASSERT;
						break;
				}/* switch */
			}/* while  */

			terminal_trip = cur_trip->exorder.fl;
			while (oc_tab[cur_trip->opcode].octype & OCT_JUMP &&
			       cur_trip->opcode != OC_CALL && cur_trip->opcode != OC_CALLSP
			       && cur_trip->operand[0].oprclass == TJMP_REF)
			{
				for (ref_trip = cur_trip->operand[0].oprval.tref
					     ;ref_trip->opcode & OCT_CGSKIP
					     ;ref_trip = ref_trip->exorder.fl)
					;

				if (ref_trip == terminal_trip)
				{
					cur_trip->opcode = OC_NOOP;
					cur_trip->operand[0] = null_operand;
					cur_trip = cur_trip->exorder.bl;
				} else
					break;
			}
			cur_trip = terminal_trip->exorder.bl;
		}/* if  */
	}/* dqloop */
}
