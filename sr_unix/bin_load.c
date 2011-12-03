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

#include "gtm_string.h"

#include "stringpool.h"
#include "stp_parms.h"
#include "gdsroot.h"
#include "gdskill.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "muextr.h"
#include "error.h"
#include "copy.h"
#include "collseq.h"
#include "util.h"
#include "op.h"
#include "gvsub2str.h"
#include "mupip_exit.h"
#include "mu_load_input.h"
#include "load.h"
#include "mvalconv.h"
#include "mu_gvis.h"
#include "gtmmsg.h"

GBLREF bool		mupip_DB_full;
GBLREF bool		mu_ctrly_occurred;
GBLREF bool		mu_ctrlc_occurred;
GBLREF bool		mupip_error_occurred;
GBLREF spdesc		stringpool;
GBLREF gd_addr		*gd_header;
GBLREF gv_key		*gv_altkey;
GBLREF gv_key		*gv_currkey;
GBLREF gv_namehead	*gv_target;
GBLREF int4		gv_keysize;
GBLREF gd_region	*gv_cur_region;
GBLREF bool             transform;

#define	BIN_PUT		0
#define BIN_BIND	1
#define ERR_COR		2

static readonly unsigned char gt_lit[] = "LOAD TOTAL";

/* starting extract file format 3, we have an extra record for each gvn, that contains the
 * collation information of the database at the time of extract. This record is transparent
 * to the user, so the semantics of the command line options, 'begin' and 'end' to MUPIP LOAD
 * will remain same. The collation header is identified in the binary extract by the fact
 * that its size is 4 bytes and no valid data record can have length 4.
 */

void		bin_call_db(int, int, int);

void bin_load(int begin, int end)
{
	unsigned char	*ptr, *cp1, *cp2, *btop, *gvkey_char_ptr, *tmp_ptr, *tmp_key_ptr;
	unsigned char	hdr_lvl, src_buff[MAX_KEY_SZ + 1], dest_buff[MAX_ZWR_KEY_SZ],
			cmpc_str[MAX_KEY_SZ + 1], dup_key_str[MAX_KEY_SZ + 1];
	unsigned char	*end_buff;
	unsigned short	len, rec_len, next_cmpc;
	int		current, last, length, iter, max_blk_siz, max_key, status, subsc_len;
	uint4		max_data_len, max_subsc_len, key_count, rec_count, global_key_count;
	boolean_t	need_xlation, new_gvn;
	rec_hdr		*rp, *next_rp;
	mval		v, tmp_mval;
	mstr		mstr_src, mstr_dest;
	collseq		*extr_collseq, *db_collseq, *save_gv_target_collseq;
	coll_hdr	extr_collhdr, db_collhdr;
	gv_key 		*tmp_gvkey;

	error_def(ERR_GVIS);
	error_def(ERR_TEXT);
	error_def(ERR_LDBINFMT);
	error_def(ERR_LOADCTRLY);
	error_def(ERR_MUNOFINISH);
	error_def(ERR_COLLTYPVERSION);
	error_def(ERR_COLLATIONUNDEF);
	error_def(ERR_OLDBINEXTRACT);

	tmp_gvkey = (gv_key *)malloc(sizeof(gv_key) + MAX_KEY_SZ - 1);
	assert(4 == sizeof(coll_hdr));
	gvinit();
	v.mvtype = MV_STR;
	len = mu_bin_get((char **)&ptr);
	if (len != BIN_HEADER_SZ)
	{
		rts_error(VARLSTCNT(1) ERR_LDBINFMT);
		mupip_exit(ERR_LDBINFMT);
	}
	/* assert the assumption that the level can be represented in a single character */
	assert(' ' == *(ptr + sizeof(BIN_HEADER_LABEL) - 3));
	if (memcmp(ptr, BIN_HEADER_LABEL, sizeof(BIN_HEADER_LABEL) - 2) || EXTR_HEADER_LEVEL(ptr) < '2')
	{				/* ignore the level check */
		rts_error(VARLSTCNT(1) ERR_LDBINFMT);
		mupip_exit(ERR_LDBINFMT);
	}
	util_out_print("Label = !AD\n", TRUE, len, ptr);
	new_gvn = FALSE;
	if ((hdr_lvl = EXTR_HEADER_LEVEL(ptr)) > '2')
	{
		len = mu_bin_get((char **)&ptr);
		if (sizeof(coll_hdr) != len)
		{
			rts_error(VARLSTCNT(5) ERR_TEXT, 2, RTS_ERROR_TEXT("Corrupt collation header"), ERR_LDBINFMT);
			mupip_exit(ERR_LDBINFMT);
		}
		extr_collhdr = *((coll_hdr *)(ptr));
		new_gvn = TRUE;
	} else
		gtm_putmsg(VARLSTCNT(3) ERR_OLDBINEXTRACT, 1, hdr_lvl - '0');
	if (!begin)
		begin = 2;
	else
	{
		if (begin < 2)
			begin = 2;
		for (iter = 2; iter < begin; iter++)
		{
			if (!(len = mu_bin_get((char **)&ptr)))
				break;
			else if (len == sizeof(coll_hdr))
			{
				extr_collhdr = *((coll_hdr *)(ptr));
				assert(hdr_lvl > '2');
				iter--;
			}
		}
		util_out_print("Beginning LOAD at #!UL\n", TRUE, begin);
	}
	max_data_len = 0;
	max_subsc_len = 0;
	key_count = 0;
	rec_count = 1;
	extr_collseq = db_collseq = NULL;
	need_xlation = FALSE;

	for (; !mupip_DB_full ;)
	{
		next_cmpc = 0;
		mupip_error_occurred = FALSE;
		if (mu_ctrly_occurred)
			break;
		if (mu_ctrlc_occurred)
		{
			util_out_print("!AD:!_  Key cnt: !UL  max subsc len: !UL  max data len: !UL", TRUE,
				LEN_AND_LIT(gt_lit), key_count, max_subsc_len, max_data_len);
			util_out_print("Last LOAD record number: !UL", TRUE, rec_count - 1);
			mu_gvis();
			util_out_print(0, TRUE);
			mu_ctrlc_occurred = FALSE;
		}
		/* reset the stringpool for every record in order to avoid garbage collection */
		stringpool.free = stringpool.base;
		if (rec_count >= end)
			break;
		if (!(len = mu_bin_get((char **)&ptr)) || mupip_error_occurred)
			break;
		else if (len == sizeof(coll_hdr))
		{
			extr_collhdr = *((coll_hdr *)(ptr));
			assert(hdr_lvl > '2');
			new_gvn = TRUE;			/* next record will contain a new gvn */
			continue;
		}
		rec_count++;
		global_key_count = 1;
		rp = (rec_hdr*)ptr;
		btop = ptr + len;
		cp1 = (unsigned char*)(rp + 1);
		v.str.addr = (char*)cp1;
		while (*cp1++) ;
		v.str.len = (char*)cp1 - v.str.addr - 1;
		if (hdr_lvl <= '2' || new_gvn)
		{
			bin_call_db(BIN_BIND, (int)gd_header, (int)&v.str);
			max_key = gv_cur_region->max_key_size;
			db_collhdr.act = gv_target->act;
			db_collhdr.ver = gv_target->ver;
			db_collhdr.nct = gv_target->nct;
		}
		GET_SHORT(rec_len, &rp->rsiz);
		if (rp->cmpc != 0 || v.str.len > rec_len || mupip_error_occurred)
		{
			bin_call_db(ERR_COR, rec_count - 1, global_key_count);
			mu_gvis();
			util_out_print(0, TRUE);
			continue;
		}
		if (new_gvn && (db_collhdr.act != extr_collhdr.act || db_collhdr.ver != extr_collhdr.ver
				|| db_collhdr.nct != extr_collhdr.nct))
		{
			if (extr_collhdr.act)
			{
				if (extr_collseq = ready_collseq((int)extr_collhdr.act))
				{
					if (!do_verify(extr_collseq, extr_collhdr.act, extr_collhdr.ver))
					{
						gtm_putmsg(VARLSTCNT(8) ERR_COLLTYPVERSION, 2, extr_collhdr.act, extr_collhdr.ver,
							ERR_GVIS, 2, gv_altkey->end - 1, gv_altkey->base);
						mupip_exit(ERR_COLLTYPVERSION);
					}
				} else
				{
					gtm_putmsg(VARLSTCNT(7) ERR_COLLATIONUNDEF, 1, extr_collhdr.act,
						ERR_GVIS, 2, gv_altkey->end - 1, gv_altkey->base);
					mupip_exit(ERR_COLLATIONUNDEF);
				}
			}
			if (db_collhdr.act)
			{
				if (db_collseq = ready_collseq((int)db_collhdr.act))
				{
					if (!do_verify(db_collseq, db_collhdr.act, db_collhdr.ver))
					{
						gtm_putmsg(VARLSTCNT(8) ERR_COLLTYPVERSION, 2, db_collhdr.act, db_collhdr.ver,
							ERR_GVIS, 2, gv_altkey->end - 1, gv_altkey->base);
						mupip_exit(ERR_COLLTYPVERSION);
					}
				} else
				{
					gtm_putmsg(VARLSTCNT(7) ERR_COLLATIONUNDEF, 1, db_collhdr.act,
						ERR_GVIS, 2, gv_altkey->end - 1, gv_altkey->base);
					mupip_exit(ERR_COLLATIONUNDEF);
				}
			}
			need_xlation = TRUE;
		} else if (new_gvn)
			need_xlation = FALSE;
		new_gvn = FALSE;
		for (; rp < (rec_hdr*)btop; rp = (rec_hdr*)((unsigned char *)rp + rec_len))
		{
			GET_SHORT(rec_len, &rp->rsiz);
			if (rec_len + (unsigned char *)rp > btop)
			{
				bin_call_db(ERR_COR, rec_count - 1, global_key_count);
				mu_gvis();
				util_out_print(0, TRUE);
				break;
			}
			cp1 =  (unsigned char*)(rp + 1);
			cp2 = gv_currkey->base + rp->cmpc;
			current = 1;
			for (;;)
			{
				last = current;
				current = *cp2++ = *cp1++;
				if (0 == last && 0 == current)
					break;
				if (cp1 > (unsigned char *) rp + rec_len ||
				    cp2 > (unsigned char *) gv_currkey + gv_currkey->top)
				{
					bin_call_db(ERR_COR, rec_count - 1, global_key_count);
					mu_gvis();
					util_out_print(0, TRUE);
					break;
				}
			}
			if (mupip_error_occurred)
				break;
			gv_currkey->end = cp2 - gv_currkey->base - 1;
			if (need_xlation)
			{
				assert(hdr_lvl >= '3');
				assert(extr_collhdr.act || db_collhdr.act || extr_collhdr.nct || db_collhdr.nct);
							/* gv_currkey would have been modified/translated in the earlier put */
				memcpy(gv_currkey->base, cmpc_str, next_cmpc);
				next_rp = (rec_hdr *)((unsigned char*)rp + rec_len);
				if ((unsigned char*)next_rp < btop)
				{
					next_cmpc = next_rp->cmpc;
					assert(next_cmpc <= gv_currkey->end);
					memcpy(cmpc_str, gv_currkey->base, next_cmpc);
				} else
					next_cmpc = 0;
							/* length of the key might change (due to nct variation),
							 * so get a copy of the original key from the extract */
				memcpy(dup_key_str, gv_currkey->base, gv_currkey->end + 1);
				gvkey_char_ptr = dup_key_str;
				while (*gvkey_char_ptr++) ;
				gv_currkey->prev = 0;
				gv_currkey->end = gvkey_char_ptr - dup_key_str;
				tmp_gvkey->top = gv_keysize;
				while (*gvkey_char_ptr)
				{
						/* get next subscript (in GT.M internal subsc format) */
					subsc_len = 0;
					tmp_ptr = src_buff;
					while (*gvkey_char_ptr)
						*tmp_ptr++ = *gvkey_char_ptr++;
					subsc_len = tmp_ptr - src_buff;
					src_buff[subsc_len] = '\0';
					if (extr_collseq)
					{
						/* undo the extract time collation */
						transform = TRUE;
						save_gv_target_collseq = gv_target->collseq;
						gv_target->collseq = extr_collseq;
					} else
						transform = FALSE;
						/* convert the subscript to string format */
					end_buff = gvsub2str(src_buff, dest_buff, FALSE);
						/* transform the string to the current subsc format */
					transform = TRUE;
					tmp_mval.mvtype = MV_STR;
                                	tmp_mval.str.addr = (char *)dest_buff;
                                	tmp_mval.str.len = end_buff - dest_buff;
					tmp_gvkey->prev = 0;
					tmp_gvkey->end = 0;
					if (extr_collseq)
						gv_target->collseq = save_gv_target_collseq;
					mval2subsc(&tmp_mval, tmp_gvkey);
						/* we now have the correctly transformed subscript */
					tmp_key_ptr = gv_currkey->base + gv_currkey->end;
					memcpy(tmp_key_ptr, tmp_gvkey->base, tmp_gvkey->end + 1);
					gv_currkey->prev = gv_currkey->end;
					gv_currkey->end += tmp_gvkey->end;
					gvkey_char_ptr++;
				}
			}
			if (gv_currkey->end >= max_key)
			{
				bin_call_db(ERR_COR, rec_count - 1, global_key_count);
				mu_gvis();
				util_out_print(0, TRUE);
				continue;
			}
			if (max_subsc_len < gv_currkey->end)
				max_subsc_len = gv_currkey->end;
			v.str.addr = (char*)cp1;
			v.str.len = rec_len - (cp1 - (unsigned char *) rp);
			if (max_data_len < v.str.len)
				max_data_len = v.str.len;
			bin_call_db(BIN_PUT, (int)&v, 0);
			if (mupip_error_occurred)
			{
				if (!mupip_DB_full)
				{
					bin_call_db(ERR_COR, rec_count - 1, global_key_count);
					util_out_print(0, TRUE);
				}
				break;
			}
			key_count++;
			global_key_count++;
		}
	}
	free(tmp_gvkey);
	mu_load_close();
	util_out_print("LOAD TOTAL!_!_Key Cnt: !UL  Max Subsc Len: !UL  Max Data Len: !UL", TRUE, key_count, max_subsc_len,
			max_data_len);
	util_out_print("Last LOAD record number: !UL\n", TRUE, rec_count);
	if(mu_ctrly_occurred)
	{
		gtm_putmsg(VARLSTCNT(1) ERR_LOADCTRLY);
		mupip_exit(ERR_MUNOFINISH);
	}
}

void bin_call_db(int routine, int parm1, int parm2)
{
	error_def(ERR_CORRUPT);

/* In order to duplicate the VMS functionality, which is to trap all errors in mupip_load_ch and
	continue in bin_load after they occur, it is necessary to call these routines from a
	subroutine due to the limitations of condition handlers and unwinding on UNIX */

	ESTABLISH(mupip_load_ch);
	switch(routine)
	{	case BIN_PUT:
			op_gvput((mval *)parm1);
			break;
		case BIN_BIND:
			gv_bind_name((gd_addr *)parm1, (mstr *)parm2);
			break;
		case ERR_COR:
			rts_error(VARLSTCNT(4) ERR_CORRUPT, 2, parm1, parm2);
			break;
	}
	REVERT;
}
