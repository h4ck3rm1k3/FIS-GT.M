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

/*
 *	SORTS_AFTER.C
 *
 *	Determine the relative sorting order of two mval's.
 *	Uses an alternate local collation sequence if present.
 *
 *	Returns:
 *		> 0  :  lhs  ]] rhs (lhs ]] rhs is true)
 *		  0  :  lhs  =  rhs (lhs ]] rhs is false)
 *		< 0  :  lha ']] rhs (lhs ]] rhs is false)
 */

#include "mdef.h"

#include "gtm_string.h"

#include "error.h"
#include "collseq.h"
#include "hashdef.h"
#include "lv_val.h"
#include "mmemory.h"
#include "do_xform.h"
#include "numcmp.h"
#include "sorts_after.h"
#include "gtm_maxstr.h"

GBLREF collseq		*local_collseq;

void DO_ALLOC_XFORM_BUFF(mstr * str) 
{
  if ((str)->len > max_lcl_coll_xform_bufsiz) { 
    if (0 == max_lcl_coll_xform_bufsiz) { 
      assert(NULL == lcl_coll_xform_buff); 
      max_lcl_coll_xform_bufsiz = MAX_STRBUFF_INIT; 
    } 
    else 
      { 
	assert(NULL != lcl_coll_xform_buff); 
	gtm_free_intern(lcl_coll_xform_buff); 
      } 
    while ((str)->len > max_lcl_coll_xform_bufsiz) 
      max_lcl_coll_xform_bufsiz += max_lcl_coll_xform_bufsiz; 
    max_lcl_coll_xform_bufsiz = MIN(MAX_STRLEN, max_lcl_coll_xform_bufsiz); 
    lcl_coll_xform_buff = (char *)gtm_malloc_intern(max_lcl_coll_xform_bufsiz); 
  }
}


void DO_MAXSTR_BUFF_FINI ()
{ 
  if (maxstr_buff[maxstr_stack_level].addr) { 
    gtm_free_intern(maxstr_buff[maxstr_stack_level].addr); 
    maxstr_buff[maxstr_stack_level].addr = NULL; 
  } 
  maxstr_buff[maxstr_stack_level].len = 0; maxstr_stack_level--; REVERT;
}


int	sorts_after (mval *lhs, mval *rhs)
{
	if (local_collseq)
	{
		int	cmp;
		int	length1;
		int	length2;
		mstr	tmstr1;
		mstr	tmstr2;
		MAXSTR_BUFF_DECL(tmp);

		ALLOC_XFORM_BUFF(&lhs->str);
		tmstr1.len = max_lcl_coll_xform_bufsiz;
		tmstr1.addr = lcl_coll_xform_buff;
		assert(NULL != lcl_coll_xform_buff);
		do_xform(local_collseq, XFORM, &lhs->str, &tmstr1, &length1);

		MAXSTR_BUFF_INIT_RET;
		tmstr2.addr = tmp;
		tmstr2.len = MAXSTR_BUFF_ALLOC(rhs->str.len, tmstr2.addr, 0);
		do_xform(local_collseq, XFORM, &rhs->str, &tmstr2, &length2);
		MAXSTR_BUFF_FINI;

		cmp = memcmp(tmstr1.addr, tmstr2.addr, length1 <= length2 ? length1 : length2);
		return cmp != 0 ? cmp : length1 - length2;
	}

	if (nm_iscan(lhs) != 0)
	{
		/* lhs is a number */
		if (nm_iscan(rhs) != 0)
			/* Both lhs and rhs are numbers */
			return numcmp(lhs, rhs);

		/* lhs is a number, but rhs is a string;
		   return false unless rhs is null */
		if (rhs->str.len == 0)
			return 1;
		return -1;
	}

	/* lhs is a string */
	if (nm_iscan(rhs) != 0)
	{
		/* lhs is a string, but rhs is a number;
		   return true unless lhs is null */
		if (lhs->str.len == 0)
			return -1;
		return 1;
	}

	/* lhs and rhs are both strings */
	return memvcmp(lhs->str.addr, lhs->str.len, rhs->str.addr, rhs->str.len);
}
