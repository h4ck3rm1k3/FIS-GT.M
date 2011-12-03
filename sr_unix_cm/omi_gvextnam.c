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
 *  omi_gvextnam.c ---
 *
 *
 *
 */

#ifndef lint
static char rcsid[] = "$Header: /cvsroot/fis-gtm/gtm/sr_unix_cm/omi_gvextnam.c,v 1.1.1.1 2001/05/16 14:01:54 marcinim Exp $";
#endif

#include "mdef.h"
#include "gtm_string.h"

#include "omi.h"
#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "error.h"
#include "parse_file.h"
#include "gbldirnam.h"
#include "dpgbldir.h"

GBLREF bool		gv_curr_subsc_null;
GBLREF gv_key		*gv_currkey;
GBLREF gd_region	*gv_cur_region;
GBLREF gd_addr		*gd_header;

int	omi_gvextnam (omi_conn *cptr, uns_short len, char *ref)
{
	bool		was_null, is_null;
	mval		v;
	char		*ptr, *end, c[MAX_FN_LEN + 1];
	omi_li		li;
	omi_si		si;
	parse_blk	pblk;
	int4		status;
	gd_segment	*cur_seg, *last_seg;


/*	Pointers into the global reference */
	ptr = ref;
	end = ref + len;

/*	Initialize part of the mval */
	v.mvtype = MV_STR;

/*	Refine the gd_addr given this environment */
	OMI_LI_READ(&li, ptr);
	if (ptr + li.value > end)
		return -OMI_ER_PR_INVGLOBREF;
	v.str.len   = li.value;
	v.str.addr  = ptr;
	cptr->ga    = zgbldir(&v);
	memset(&pblk, 0, sizeof(pblk));
	pblk.buffer = c;
	pblk.buff_size = MAX_FBUFF;
	pblk.def1_buf = DEF_GDR_EXT;
	pblk.def1_size = sizeof(DEF_GDR_EXT) - 1;
	status = parse_file(&v.str, &pblk);

	/* for all segments insert the full path in the segment fname */
	cur_seg = cptr->ga->segments;
	last_seg  = cur_seg + cptr->ga->n_segments;
	for( ; cur_seg < last_seg ; cur_seg++)
	{
		if ('/' != cur_seg->fname[0])
		{	/* doesn't contains full path ; specify full path */
			memmove(&cur_seg->fname[0] + pblk.b_dir, cur_seg->fname, cur_seg->fname_len);
			memcpy(cur_seg->fname, pblk.l_dir, pblk.b_dir);
			cur_seg->fname_len += pblk.b_dir;
		}
	}
	ptr += li.value;
	/* Refine the gd_addr given this name */
	OMI_SI_READ(&si, ptr);
	if (si.value <= 1  ||  *ptr != '^')
		return -OMI_ER_PR_INVGLOBREF;
	ptr++;
	si.value--;
	if (ptr + si.value > end)
		return -OMI_ER_PR_INVGLOBREF;
	v.str.len   = si.value;
	v.str.addr  = ptr;
	gd_header   = cptr->ga;
	gv_bind_name(cptr->ga, &v.str);
	ptr        += si.value;
	/* Refine the gd_addr given these subscripts */
	was_null = is_null  = FALSE;
	while (ptr < end)
	{
		was_null  |= is_null;
		OMI_SI_READ(&si, ptr);
		if (ptr + si.value > end)
			return -OMI_ER_PR_INVGLOBREF;
		v.mvtype   = MV_STR;
		v.str.len  = si.value;
		v.str.addr = ptr;
		is_null    = (si.value == 0);
		mval2subsc(&v, gv_currkey);
		ptr       += si.value;
	}
	gv_curr_subsc_null = is_null;
	if (was_null  &&  !gv_cur_region->null_subs)
		return -OMI_ER_DB_INVGLOBREF;
	return 0;
}
