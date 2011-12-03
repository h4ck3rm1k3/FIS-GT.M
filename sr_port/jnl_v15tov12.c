/****************************************************************
 *								*
 *	Copyright 2003 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "gtm_string.h"

#include <stddef.h>

#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsblk.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "jnl.h"
#include "copy.h"
#include "iosp.h"
#include "repl_filter.h"
#include "repl_errno.h"
#include "jnl_typedef.h"

int4 v12_jnl_record_length(jnl_record *rec, int4 top);

LITREF int      	v12_jnl_fixed_size[];
LITREF	boolean_t	jrt_is_replicated[JRT_RECTYPES];
LITREF	int		jrt_update[JRT_RECTYPES];

/* Convert a transaction from jnl version  15 (V.4.4-002) to 12  (GT.M versions V4.3-000 through V4.3-001E) */
int jnl_v15tov12(uchar_ptr_t jnl_buff, uint4 *jnl_len, uchar_ptr_t conv_buff, uint4 *conv_len, uint4 conv_bufsiz)
{
	unsigned char		*jb, *cb, *cstart, *jstart, *ptr;
	enum	jnl_record_type	rectype;
	int			status, reclen;
	unsigned short		key_len;
	unsigned int		long_data_len, jlen, total_data, nzeros, conv_reclen, clen_without_sfx;
	unsigned int		src_total_key, des_total_key;
	jrec_prefix 		*prefix;
	mstr_len_t		data_len;

	jb = jnl_buff;
	cb = conv_buff;
	status = SS_NORMAL;
	jlen = *jnl_len;
	assert(0 == ((uint4)jb % sizeof(uint4)));
   	while (JREC_PREFIX_SIZE <= jlen)
	{
		assert(0 == ((uint4)jb % sizeof(uint4)));
		prefix = (jrec_prefix *)jb;
		rectype = prefix->jrec_type;
		cstart = cb;
		jstart = jb;
   		if (0 != (reclen = prefix->forwptr))
		{
   			if (reclen <= jlen)
			{
				des_total_key = src_total_key = total_data = 0;
				assert(IS_REPLICATED(rectype));
				if (IS_ZTP(rectype))
					GTMASSERT;	/* ZTP not supported */
				if (IS_SET_KILL_ZKILL(rectype))
				{
					ptr = jb + FIXED_UPD_RECLEN;
					GET_JNL_STR_LEN(key_len, ptr);
					assert(key_len <= MAX_KEY_SZ);
					src_total_key = key_len + sizeof(jnl_str_len_t);
					des_total_key = key_len + sizeof(unsigned short);
					if (IS_SET(rectype))
					{
						ptr = jb + FIXED_UPD_RECLEN + src_total_key;
						GET_MSTR_LEN(long_data_len, ptr);
						total_data = long_data_len + sizeof(mstr_len_t);
					}
				}
				clen_without_sfx = ROUND_UP(V12_JREC_PREFIX_SIZE + v12_jnl_fixed_size[rectype] + des_total_key +
							    total_data, V12_JNL_REC_START_BNDRY);
				conv_reclen = clen_without_sfx + V12_JREC_SUFFIX_SIZE;
				if (cb - conv_buff + conv_reclen > conv_bufsiz)
				{
					repl_errno = EREPL_INTLFILTER_NOSPC;
					status = -1;
					break;
				}
				if (IS_SET_KILL_ZKILL(rectype))
				{
					memset(cb, 0, V12_JREC_PREFIX_SIZE + V12_MUMPS_NODE_OFFSET);
					*cb = (char)rectype;
					cb += V12_JREC_PREFIX_SIZE + V12_JNL_SEQNO_OFFSET;
					memcpy(cb, jb + V15_JNL_SEQNO_OFFSET, sizeof(seq_num));
					cb += sizeof(seq_num);
					memset(cb, 0, 2 * sizeof(uint4));
					cb += 2 * sizeof(uint4);
					if (IS_FENCED(rectype))
					{
						memcpy(cb, jb + V15_JNL_SEQNO_OFFSET, sizeof(seq_num));
						cb += sizeof(seq_num);
						memset(cb, 0, 8);
						cb += 8;
					}
					jb += FIXED_UPD_RECLEN;
					PUT_SHORT(cb, key_len);
					cb += sizeof(unsigned short);
					jb += sizeof(jnl_str_len_t);
					memcpy(cb, jb, key_len);
					cb += key_len;
					jb += key_len;
					if (IS_SET(rectype))
					{
						data_len = (mstr_len_t)long_data_len;
						PUT_MSTR_LEN(cb, data_len);
						cb += sizeof(mstr_len_t);
						jb += sizeof(mstr_len_t);
						memcpy(cb, jb, data_len);
						cb += data_len;
						jb += data_len;
					}
				} else if (IS_COM(rectype))
				{
					memset(cb, 0, V12_JREC_PREFIX_SIZE + v12_jnl_fixed_size[rectype]);
					*cb = (char)rectype;
					cb += V12_JREC_PREFIX_SIZE + V12_JNL_SEQNO_OFFSET;
					memcpy(cb, jb + V15_JNL_SEQNO_OFFSET, sizeof(seq_num));
					cb += sizeof(seq_num);
					cb += 2 * sizeof(uint4); /* for tc_recov_short_time and ts_recov_short_time */
					memcpy(cb, jb + V15_JNL_SEQNO_OFFSET, sizeof(seq_num));
					cb += sizeof(seq_num);
					memcpy(cb, jb + V15_TCOM_PARTICIPANTS_OFFSET, sizeof(uint4));
					cb += sizeof(uint4) + sizeof(uint4);	/* participants + ts_short_time */
				} else
					assert(FALSE);
				nzeros = (cstart + clen_without_sfx - cb);
				if (nzeros > 0)
				{
					assert(V12_JNL_REC_START_BNDRY > nzeros);
					memset(cb, 0, nzeros);
					cb += nzeros;
				}
				jb = jstart + reclen;
				memset(cb, 0, V12_JREC_SUFFIX_SIZE);
				((pre_v15_jrec_suffix *)cb)->suffix_code = PRE_V15_JNL_REC_TRAILER;
				/* We cannot calculate backptr */
				cb += V12_JREC_SUFFIX_SIZE;
				assert(cb == cstart + conv_reclen);
				jlen -= reclen;
				continue;
			}
			repl_errno = EREPL_INTLFILTER_INCMPLREC;
			status = -1;
			break;
		}
		repl_errno = EREPL_INTLFILTER_BADREC;
		status = -1;
		break;
	}
	if (0 != jlen)
	{
		repl_errno = EREPL_INTLFILTER_INCMPLREC;
		status = -1;
	}
	assert(0 == jlen || -1 == status);
	*jnl_len = jb - jnl_buff;
	*conv_len = cb - conv_buff;
	return(status);
}
