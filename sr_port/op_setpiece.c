/****************************************************************
 *								*
 *	Copyright 2001, 2004 Sanchez Computer Associates, Inc.	*
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
#include "op.h"
#include "matchc.h"

GBLREF spdesc stringpool;

/*
 * ----------------------------------------------------------
 * NOTE:
 * 	Following code is a transliteration of VAX macro.
 *	No attempts to convert this code to native C
 *	style was performed. Goto's were reduced to minimum
 *	without modifying the original code structure.
 *	Labels of format lnn in the comments are those of the
 *	original code.
 * ----------------------------------------------------------
 */

/*
 * ----------------------------------------------------------
 * Set piece procedure.
 * Set pieces first through last to expr.
 *
 * Arguments:
 *	src	- source mval
 *	del	- delimiter string mval
 *	expr	- expression string mval
 *	first	- starting index in source mval to be set
 *	last	- last index
 *	dst	- destination mval where the result is saved.
 *
 * Return:
 *	none
 * ----------------------------------------------------------
 */
void op_setpiece(mval *src, mval *del, mval *expr, int4 first, int4 last, mval *dst)
{
	int match_res, len, src_len, str_len, delim_cnt;
	int first_src_ind, second_src_ind;
	unsigned char *match_ptr, *src_str, *str_addr, *tmp_str;
	error_def(ERR_MAXSTRLEN);

	/* --- code start --- */

	if (--first < 0)
		first = 0;

	second_src_ind = last - first;
	MV_FORCE_STR(del);

	/* Null delimiter */
	if (del->str.len == 0)
	{
		if (first && src->mvtype)
		{
			/* concat src & expr to dst */
			op_cat(VARLSTCNT(3) dst, src, expr);
			return;
		}
		MV_FORCE_STR(expr);
		*dst = *expr;
		return;
	}

	/* Valid delimiter */
	MV_FORCE_STR(expr);
	if (! MV_DEFINED(src))
	{
		first_src_ind = 0;
		second_src_ind = -1;
		goto moveit;
	}

	/* We have a valid src with something in it */
	MV_FORCE_STR(src);
	src_str = (unsigned char *)src->str.addr;
	src_len = src->str.len;
	match_ptr = src_str;


	/* skip all pieces before start one */
	while (first > 0)
	{
		match_ptr = matchc(del->str.len, (uchar_ptr_t)del->str.addr,
			src_len, src_str, &match_res);
		src_len -= (match_ptr - src_str);
		src_str = match_ptr;
		/* if match not found */
		if (match_res)
			goto l80;
		first--;
	}
	/*
	 * skip # delimiters this piece will replace, e.g. if we are setting
	 * pieces 2 - 4, then the pieces 2-4 will be replaced by one piece - expr.
	 */

	first_src_ind = match_ptr - (unsigned char *)src->str.addr;
	do {
		match_ptr = matchc(del->str.len, (uchar_ptr_t)del->str.addr,
			src_len, src_str, &match_res);
		src_len -= (match_ptr - src_str);
		src_str = match_ptr;
			/* if match not found */
		if (match_res)
			goto l90;
	} while (--second_src_ind > 0);

	second_src_ind = match_ptr - (unsigned char *)src->str.addr - del->str.len;
	goto moveit;
l80:
	first_src_ind = match_ptr - (unsigned char *)src->str.addr;
l90:
	second_src_ind = -1;

moveit:
	delim_cnt = first;

	/* Calculate total string len. */
	str_len = expr->str.len + (first_src_ind + del->str.len * delim_cnt);
	/* add len. of trailing chars past insertion point */
	if (second_src_ind >= 0)
		str_len += (src->str.len - second_src_ind);

	if (str_len > MAX_STRLEN)
	{
		rts_error(VARLSTCNT(1) ERR_MAXSTRLEN);
		return;
	}

	if (str_len > (stringpool.top - stringpool.free))
		stp_gcol(str_len);
	str_addr = stringpool.free;

	/* copy prefix */
	if (first_src_ind)
	{
		memcpy(str_addr, src->str.addr, first_src_ind);
		str_addr += first_src_ind;
	}

	/* copy delimiters */
	while (delim_cnt-- > 0)
	{
		memcpy(str_addr, del->str.addr, del->str.len);
		str_addr += del->str.len;
	}

	/* copy expression */
	memcpy(str_addr, expr->str.addr, expr->str.len);
	str_addr += expr->str.len;

	/* copy trailing pieces */
	if (second_src_ind >= 0)
	{
		len = src->str.len - second_src_ind;
		tmp_str = (unsigned char *)src->str.addr + second_src_ind;
		memcpy(str_addr, tmp_str, len);
		str_addr += len;
	}

	assert(str_addr - stringpool.free == str_len);
	dst->mvtype = MV_STR;
	dst->str.len = str_addr - stringpool.free;
	dst->str.addr = (char *)stringpool.free;
	stringpool.free = str_addr;
	return;
}

