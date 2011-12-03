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

#include <stddef.h>

#include "gtm_string.h"

#include "copy.h"
#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "cmidef.h"
#include "hashdef.h"
#include "cmmdef.h"
#include "gvcst_zprevious.h"
#include "gvcst_data.h"
#include "gv_xform_key.h"
#include "gtcm_find_region.h"
#include "gtcm_bind_name.h"
#include "gtcmtr_zprevious.h"

GBLREF connection_struct *curr_entry;
GBLREF gv_namehead	*gv_target;
GBLREF gv_namehead	*reset_gv_target;
GBLREF gv_key		*gv_currkey;
GBLREF gv_key		*gv_altkey;
GBLREF sgmnt_addrs	*cs_addrs;

bool gtcmtr_zprevious(void)
{
	boolean_t	found, is_null;
	unsigned char	*ptr, regnum;
	unsigned short	top, old_top;
	unsigned short	len, tmp_len;
	gv_key		*save_key;
	cm_region_list	*reg_ref;

	ptr = curr_entry->clb_ptr->mbf;
	assert(CMMS_Q_PREV == *ptr);
	ptr++;
	GET_USHORT(len, ptr);
	ptr += sizeof(short);
	regnum = *ptr++;
	reg_ref = gtcm_find_region(curr_entry, regnum);
	len--;	/* subtract size of regnum */
	assert(0 == offsetof(gv_key, top));
	GET_USHORT(old_top, ptr); /* old_top = ((gv_key *)ptr)->top; */
	CM_GET_GVCURRKEY(ptr, len);
	gtcm_bind_name(reg_ref->reghead, FALSE); /* gtcm_bind_name sets gv_target; do not use gv_target before gtcm_bind_name */
	if ((gv_target->collseq || gv_target->nct) && gv_currkey->prev)
	{
		if (is_null = (STR_SUB_PREFIX == gv_currkey->base[gv_currkey->end - 2] &&
			       STR_SUB_PREFIX == gv_currkey->base[gv_currkey->end - 3]))
		{ /* last subscript of incoming key is a NULL subscript */
			gv_currkey->base[gv_currkey->end - 2] = KEY_DELIMITER;
			gv_currkey->end--;
		}
		gv_xform_key(gv_currkey, FALSE);
	}
	if (gv_currkey->prev)
	{
		if ((gv_target->collseq || gv_target->nct) && is_null)
		{
			assert(STR_SUB_PREFIX == gv_currkey->base[gv_currkey->end - 2]); /* null subsc -> non null not allowed */
			gv_currkey->base[gv_currkey->prev + 1] = STR_SUB_PREFIX;
			gv_currkey->base[++(gv_currkey->end)] = 0;
		}
		found = (0 == gv_target->root) ? FALSE : gvcst_zprevious();
	} else
	{	/* name level */
		assert(2 <= gv_currkey->end);			/* at least one character of name, and 2 <NUL> delimiters */
		assert((sizeof(mident) + 2) >= gv_currkey->end);	/* no more than an mident, and two <NUL> delimiters */
		assert(INVALID_GV_TARGET == reset_gv_target);
		for (;  ;)
		{
			reset_gv_target = gv_target;	/* for restoration, just in case something goes wrong before
							 * gtcm_bind_name() is called */
			gv_target = cs_addrs->dir_tree;
			found = gvcst_zprevious();
			if (!found)
				break;
			assert(2 <= gv_altkey->end);			/* at least one character of name and a <NUL> delimiter */
 			assert((sizeof(mident) + 2) >= gv_currkey->end);	/* no more than an mident & two <NUL> delimiters */
			save_key = gv_currkey;
			gv_currkey = gv_altkey;
			gv_altkey = save_key;
			gtcm_bind_name(reg_ref->reghead, TRUE);
			reset_gv_target = INVALID_GV_TARGET;
			if ((0 != gv_target->root) && (0 != gvcst_data()))
			{
				save_key = gv_currkey;
				gv_currkey = gv_altkey;
				gv_altkey = save_key;
				break;
			}
		}
	}
	if (!found)
		len = 0;
	else
	{
		gv_altkey->prev = gv_currkey->prev;
		if (gv_target->nct || gv_target->collseq)
			gv_xform_key(gv_altkey, TRUE);
		/* len = sizeof(gv_key) + gv_altkey->end; */
		len = gv_altkey->end + sizeof(unsigned short) + sizeof(unsigned short) + sizeof(unsigned short) +
		      sizeof(char);
	}
	ptr = curr_entry->clb_ptr->mbf;
	*ptr++ = CMMS_R_PREV;
	tmp_len = len + 1;
	PUT_USHORT(ptr, tmp_len);
	ptr += sizeof(short);
	*ptr++ = regnum;
	if (len)
	{
		/* memcpy(ptr, gv_altkey, len); */ /* this memcpy modified to the following PUTs and memcpy; vinu, 07/18/01 */
		/* we are goint to restore top from old_top, why even bother setting it now? vinu, 07/18/01 */
		/* PUT_USHORT(ptr, gv_altkey->top); */
		PUT_USHORT(ptr + sizeof(unsigned short), gv_altkey->end);
		PUT_USHORT(ptr + sizeof(unsigned short) + sizeof(unsigned short), gv_altkey->prev);
		memcpy(ptr + sizeof(unsigned short) + sizeof(unsigned short) + sizeof(unsigned short), gv_altkey->base,
		       len - sizeof(unsigned short) - sizeof(unsigned short) - sizeof(unsigned short));
	}
	PUT_USHORT(ptr, old_top); /* ((gv_key *)ptr)->top = old_top; */
	curr_entry->clb_ptr->cbl = sizeof(unsigned char) + sizeof(unsigned short) + sizeof(unsigned char) + len;
	reset_gv_target = INVALID_GV_TARGET;
	return TRUE;
}
