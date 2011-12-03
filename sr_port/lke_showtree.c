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

/*
 * -----------------------------------
 * lke_showtree	: displays a lock tree
 * used in	: lke_show.c
 * -----------------------------------
 */

#include "mdef.h"

#include <signal.h>

#include "gtm_string.h"
#include "gtm_stdio.h"

#include "mlkdef.h"
#include "cmidef.h"
#include "lke.h"

#define KDIM	64		/* max number of subscripts */
#define	SDIM	256		/* max subscript size */

GBLREF VSIG_ATOMIC_T	util_interrupt;

void lke_show_memory(mlk_shrblk_ptr_t bhead, char *prefix)
{
	mlk_shrblk_ptr_t	b, bnext;
	mlk_shrsub_ptr_t	dsub;
	char			temp[SDIM];
	char			new_prefix[KDIM+2];

	SPRINTF(new_prefix, "	%s", prefix);
	for (b = bhead, bnext = 0; bnext != bhead; b = bnext)
	{
		dsub = (mlk_shrsub_ptr_t)R2A(b->value);
		memcpy(temp, dsub->data, dsub->length);
		temp[dsub->length] = '\0';
		PRINTF("%s%s : [shrblk] %x : [shrsub] %x\n", prefix, temp, b, dsub);
		if (b->children)
			lke_show_memory((mlk_shrblk_ptr_t)R2A(b->children), new_prefix);
		bnext = (mlk_shrblk_ptr_t)R2A(b->rsib);
	}
}


bool	lke_showtree(struct CLB 	*lnk,
		     mlk_shrblk_ptr_t	tree,
		     bool 		all,
		     bool 		wait,
		     long 		pid,
		     mstr 		one_lock,
		     bool 		memory)
{
	mlk_shrblk_ptr_t	node, start[KDIM];
	unsigned char	subscript_offset[KDIM];
	static char	name_buffer[SDIM];
	static MSTR_DEF(name, 0, name_buffer);
	int		depth = 0;
	bool		locks = FALSE;

	error_def(ERR_CTRLC);

	if (memory)
	{
		lke_show_memory(tree, "	");
		return TRUE;
	}

	node = start[0]
	     = tree;
	subscript_offset[0] = 0;

	for (;;)
	{
		name.len = subscript_offset[depth];

		/* Display the lock node */
		locks = lke_showlock(lnk, node, &name, all, wait, TRUE, pid, one_lock)
			|| locks;

	  	/* Move to the next node */
		if (node->children == 0)
		{
			/* This node has no children, so move to the right */
			node = (mlk_shrblk_ptr_t)R2A(node->rsib);
			while (node == start[depth])
			{
				/* There are no more siblings to the right at this depth,
				   so move up and then right */
				if (node->parent == 0)
				{
					/* We're already at the top, so we're done */
					assert(depth == 0);
					return locks;
				}
				--depth;
				node = (mlk_shrblk_ptr_t)R2A(((mlk_shrblk_ptr_t)R2A(node->parent))->rsib);
			}
		}
		else
		{
			/* This node has children, so move down */
			++depth;
			node = start[depth]
			     = (mlk_shrblk_ptr_t)R2A(node->children);
			subscript_offset[depth] = name.len;
		}
		if (util_interrupt)
			rts_error(VARLSTCNT(1) ERR_CTRLC);
	}
}
