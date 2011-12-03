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
#include "gvcst_query.h"
#include "gtcm_find_region.h"
#include "gtcm_bind_name.h"
#include "gtcmtr_query.h"
#include "gv_xform_key.h"
#include "gvcst_queryget.h"

GBLREF connection_struct *curr_entry;
GBLREF gv_namehead	*gv_target;
GBLREF gv_key		*gv_currkey;
GBLREF gv_key		*gv_altkey;

bool gtcmtr_query(void)
{
	unsigned char	*ptr, *gv_key_top_ptr, regnum;
	unsigned short	top, old_top;
	unsigned short	key_len, tmp_len, tot_val_len;
	cm_region_list	*reg_ref;
	mval		val;
	boolean_t	found, was_null;
	uint4		msg_len;

	ptr = curr_entry->clb_ptr->mbf;
	assert(CMMS_Q_QUERY == *ptr);
	ptr++;
	GET_USHORT(tmp_len, ptr);
	ptr += sizeof(short);
	regnum = *ptr++;
	reg_ref = gtcm_find_region(curr_entry, regnum);
	tmp_len--;	/* subtract size of regnum */
	assert(0 == offsetof(gv_key, top));
	GET_USHORT(old_top, ptr); /* old_top = ((gv_key *)ptr)->top; */
	CM_GET_GVCURRKEY(ptr, tmp_len);
	assert(1 == gv_currkey->base[gv_currkey->end - 2]);
	assert(0 != gv_currkey->prev || KEY_DELIMITER == gv_currkey->base[gv_currkey->end - 3]);
	gtcm_bind_name(reg_ref->reghead, FALSE); /* gtcm_bind_name sets gv_target; do not use gv_target before gtcm_bind_name */
	if (gv_target->nct || gv_target->collseq)
	{ /* undo client appended 01 00 00 before user provided collation routine gets control */
		if (0 == gv_currkey->prev ||
			1 != gv_currkey->base[gv_currkey->prev] || 0 != gv_currkey->base[gv_currkey->prev + 1])
		{ /* name level $Q, or last subscript of incoming key not null */
			gv_currkey->end -= 2;
			gv_currkey->base[gv_currkey->end] = KEY_DELIMITER;
			DEBUG_ONLY(was_null = FALSE;)
		} else
		{ /* last subscript of incoming key null */
			gv_currkey->base[gv_currkey->prev] = STR_SUB_PREFIX;
			DEBUG_ONLY(was_null = TRUE;)
		}
		gv_xform_key(gv_currkey, FALSE);
		/* redo append 01 00 00 now that we are done with collation */
		if (0 == gv_currkey->prev || KEY_DELIMITER != gv_currkey->base[gv_currkey->end - 3] ||
			STR_SUB_PREFIX != gv_currkey->base[gv_currkey->end - 2])
		{
			assert(!was_null); /* null to non null transformation not allowed */
			gv_currkey->base[gv_currkey->end++] = 1;
			gv_currkey->base[gv_currkey->end++] = 0;
			gv_currkey->base[gv_currkey->end] = 0;
		} else
			gv_currkey->base[gv_currkey->prev] = 1;
	}
	found = (0 != gv_target->root) ? (curr_entry->query_is_queryget ? gvcst_queryget(&val) : gvcst_query()) : FALSE;
	if (found)
	{
		if (gv_target->nct || gv_target->collseq)
			gv_xform_key(gv_altkey, TRUE);
		/* key_len = sizeof(gv_key) + gv_altkey->end; */
		key_len = gv_altkey->end + sizeof(unsigned short) + sizeof(unsigned short) + sizeof(unsigned short) + sizeof(char);
		tot_val_len = (curr_entry->query_is_queryget ? val.str.len + sizeof(unsigned short) : 0);
		/* ushort <- uint4 assignment lossy? */
		assert((uint4)tot_val_len == (curr_entry->query_is_queryget ? val.str.len + sizeof(unsigned short) : 0));
	} else
		key_len = tot_val_len = 0;
	msg_len = sizeof(unsigned char) + sizeof(unsigned short) + sizeof(unsigned char) + key_len + tot_val_len;
	if (msg_len > curr_entry->clb_ptr->mbl)
		cmi_realloc_mbf(curr_entry->clb_ptr, msg_len);
	ptr = curr_entry->clb_ptr->mbf;
	*ptr++ = CMMS_R_QUERY;
	tmp_len = key_len + 1;
	PUT_USHORT(ptr, tmp_len);
	ptr += sizeof(unsigned short);
	*ptr++ = regnum;
	gv_key_top_ptr = ptr;
	if (found)
	{
		/* memcpy(ptr, gv_altkey, key_len); */ /* this memcpy modified to the following PUTs and memcpy; vinu, 07/18/01 */

		/* we are going to set ((gv_key *)ptr)->top to old_top, why even bother setting it to now to gv_altkey->top?
		 * vinu, 07/18/01 */

		/* PUT_USHORT(ptr, gv_altkey->top); */
		ptr += sizeof(unsigned short);
		PUT_USHORT(ptr, gv_altkey->end);
		ptr += sizeof(unsigned short);
		PUT_USHORT(ptr, gv_altkey->prev);
		ptr += sizeof(unsigned short);
		memcpy(ptr, gv_altkey->base, key_len - sizeof(unsigned short) - sizeof(unsigned short) - sizeof(unsigned short));
		ptr += (key_len - sizeof(unsigned short) - sizeof(unsigned short) - sizeof(unsigned short));
		if (curr_entry->query_is_queryget)
		{
			tmp_len = tot_val_len - sizeof(unsigned short);
			PUT_USHORT(ptr, tmp_len);
			ptr += sizeof(unsigned short);
			memcpy(ptr, val.str.addr, tmp_len);
		}
	}
	PUT_USHORT(gv_key_top_ptr, old_top); /* ((gv_key *)ptr)->top = old_top; */
	curr_entry->clb_ptr->cbl = msg_len;
	return TRUE;
}
