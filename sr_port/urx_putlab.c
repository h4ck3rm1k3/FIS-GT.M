/****************************************************************
 *								*
 *	Copyright 2001, 2002 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"
#include "gtm_string.h"
#include "rtnhdr.h" /* For urx.h */
#include "urx.h"

void urx_putlab (char *lab, int lablen, urx_rtnref *rtn, char *addr)
{
	urx_labref	*lp0, *lp1, *tmplp;
	boolean_t	found;
	int		c;
	urx_addr	*tmpap;

	found = FALSE;
	lp0 = (urx_labref *)rtn;
	lp1 = ((urx_rtnref *) lp0)->lab;
	/* Locate the given label on this routine's unresolved label chain. If not found, we will
	   create a new element and insert it on the chain. When we have a label node, allocate and
	   add an address reference for the supplied address. This address will receive the resolved
	   value when the label becomes resolved. Note that labels are ordered on this chain alphabetically
	   within label name size ... i.e. all sorted 1 character labels followed
	   by all sorted 2 character labels, etc. Note also that the layouts of urx_rtnref and urx_labref
	   are critical as the "next" field in labref is at the same offset as the "lab" anchor in the
	   rtnref block allowing urx_rtnref to be cast and serve as an anchor for the labref chain.
	*/
	while (lp1 != 0)
	{
		c = lablen - lp1->len;
		if (!c)
			c = memcmp(lab, lp1->name.c, lablen);
		if (c > 0)
		{
			lp0 = lp1;
			lp1 = lp0->next;
		} else
		{
			if (c == 0)
				found = TRUE;
			break;
		}
	}
	if (lp0 == (urx_labref *)rtn)
		assert(((urx_rtnref *)lp0)->lab == lp1);
	else
		assert(lp0->next == lp1);

	if (!found)
	{	/* Add new label name to list */
		tmplp = (urx_labref *)gtm_malloc_intern(sizeof(urx_labref));
		tmplp->len = lablen;
		memcpy(tmplp->name.c, lab, lablen);
		tmplp->addr = 0;
		tmplp->next = lp1;
		if (lp0 == (urx_labref *)rtn)
			((urx_rtnref *)lp0)->lab = tmplp;
		else
			lp0->next = tmplp;
		lp1 = tmplp;
	}
	assert(lp1 != 0);

	tmpap = (urx_addr *)gtm_malloc_intern(sizeof(urx_addr));
	tmpap->next = lp1->addr;
	tmpap->addr = (int4 *)addr;
	lp1->addr = tmpap;
	return;
}
