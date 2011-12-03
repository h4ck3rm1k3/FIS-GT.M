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

#include <errno.h>
#include "gtm_termio.h"

#include "io.h"
#include "iottdef.h"
#include "gtmio.h"
#include "eintr_wrappers.h"
#include "setterm.h"
#include "gtm_isanlp.h"

void  resetterm(io_desc *iod)
{
	int		status;
	int		save_errno;
	struct termios 	t;
	d_tt_struct	*ttptr;
	error_def(ERR_TCSETATTR);

	ttptr =(d_tt_struct *) iod->dev_sp;
	if (ttptr->ttio_struct)
	{
	        t = *ttptr->ttio_struct;
		Tcsetattr(ttptr->fildes, TCSANOW, &t, status);
		if (status != 0)
		{
			save_errno = errno;
			if (gtm_isanlp(ttptr->fildes) == 0)
				rts_error(VARLSTCNT(4) ERR_TCSETATTR, 1, ttptr->fildes, save_errno);
		}
	}
	return;
}
