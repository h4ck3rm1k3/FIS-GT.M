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

#include "underr.h"
#include <stdarg.h>
#include "hashdef.h"
#include "lv_val.h"
#include "undx.h"

GBLDEF bool	undef_inhibit = 0;
LITREF mval	literal_null;

void	underr_old (int something,...)
//va_dcl
{
	mval		*start;
	mident		name;
	unsigned char	*end;
	va_list		var;		/* this is a dummy so we can pass a va_list to undx */
	error_def(ERR_UNDEF);

	va_start (var,something);
	start = va_arg(var, mval *);
	if (start && undef_inhibit)
		*start = literal_null;
	else
	{
		end = format_lvname((lv_val *)start, (uchar_ptr_t)name.c, sizeof(mident));
		rts_error(VARLSTCNT(4) ERR_UNDEF, 2, end - ((unsigned char *) &name), &name);
	}
	return;
}


void underr (mval * var)
{
  // mval *start;
  mident name;
  unsigned char *end;
  // va_list var;
  extern const int ERR_UNDEF;
  
  // __builtin_va_start(var,something);
  // start = __builtin_va_arg(var,mval *);
  if (var && undef_inhibit)
    {
      *var = literal_null;
    }
  else
    {
      end = format_lvname((lv_val *)var, (uchar_ptr_t)name.c, sizeof(mident));
      rts_error(4, ERR_UNDEF, 2, end - ((unsigned char *) &name), &name);
    }
  return;
}
