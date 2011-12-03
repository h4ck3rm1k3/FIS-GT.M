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
#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "cdb_sc.h"
#include "copy.h"
#include "gvcst_expand_key.h"

GBLREF unsigned int	t_tries;

enum cdb_sc	gvcst_expand_key(blk_hdr_ptr_t bp, int4 rec_top, gv_key *key)
{
	unsigned short	temp_ushort;
	int4		r_offset;
	rec_hdr_ptr_t	rp, rtop;
	sm_uc_ptr_t	p;
	unsigned char	*kbase, *kend, *kprv, *ktop, last, current;

	assert(sizeof(rec_hdr) <= sizeof(blk_hdr));
	kbase = kend = key->base;
	ktop = &key->base[key->top];
	rp = (rec_hdr_ptr_t)bp;
	rtop = (rec_hdr_ptr_t)((sm_uc_ptr_t)bp + rec_top);
	for (r_offset = sizeof(blk_hdr);  ;  GET_USHORT(temp_ushort, &rp->rsiz), r_offset = temp_ushort)
	{
		/* WARNING:  Assumes that sizeof(rec_hdr) <= sizeof(blk_hdr)	*/
		if (r_offset < sizeof(rec_hdr))
		{
			assert(CDB_STAGNATE > t_tries);
			return cdb_sc_r2small;
		}
		rp = (rec_hdr_ptr_t)((sm_uc_ptr_t)rp + r_offset);
		if (rp > rtop)
		{
			assert(CDB_STAGNATE > t_tries);
			return cdb_sc_rmisalign;
		}
		current = 1;
		kend = kbase + rp->cmpc;
		p = (sm_uc_ptr_t)(rp + 1);
		for (;;)
		{
			if (kend >= ktop)
			{
				assert(CDB_STAGNATE > t_tries);
				return cdb_sc_keyoflow;
			}
			last = current;
			*kend++ = current = *p++;
			if (last == 0)
			{
				if (current == 0)
					break;
				else
					kprv = kend - 1;	/* start of last key */
			}
		}
		if (rp == rtop)
		{
			key->end = kend - kbase - 1;
			key->prev = kprv - kbase;
			return cdb_sc_normal;
		}
		kprv = kend - 1;	/* start of last key */
	}
}
