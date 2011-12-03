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

#include <errno.h>
#include "gtm_unistd.h"
#include "gtm_fcntl.h"
#include "gtm_stat.h"
#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "gtm_stdio.h"
#include "gtm_stdlib.h"
#include "gtm_string.h"
#include "gtmio.h"
#include "iosp.h"
#include "eintr_wrappers.h"
#include "file_head_read.h"
#include "gtmmsg.h"

/*
 * This is a plain way to read file header.
 * User needs to take care of concurrency issue etc.
 * Parameters :
 *	fn : full name of a database file.
 *	header: Pointer to database file header structure (may not be in shared memory)
 */
boolean_t file_head_read(char *fn, sgmnt_data_ptr_t header)
{
	int 		save_errno, fd, header_size;
	struct stat	stat_buf;

	error_def(ERR_DBFILOPERR);
	error_def(ERR_DBNOTGDS);

	header_size = sizeof(sgmnt_data);
	OPENFILE(fn, O_RDONLY, fd);
	if (-1 == fd)
	{
		save_errno = errno;
		gtm_putmsg(VARLSTCNT(5) ERR_DBFILOPERR, 2, LEN_AND_STR(fn), save_errno);
		return FALSE;
	}
	FSTAT_FILE(fd, &stat_buf, save_errno);
	if (-1 == save_errno)
	{
		save_errno = errno;
		gtm_putmsg(VARLSTCNT(5) ERR_DBFILOPERR, 2, LEN_AND_STR(fn), save_errno);
 		CLOSEFILE(fd, save_errno);
		return FALSE;
	}
	if (!S_ISREG(stat_buf.st_mode) || stat_buf.st_size < header_size)
	{
		gtm_putmsg(VARLSTCNT(5) ERR_DBNOTGDS, 2, LEN_AND_STR(fn));
 		CLOSEFILE(fd, save_errno);
		return FALSE;
	}
	LSEEKREAD(fd, 0, header, header_size, save_errno);
	if (0 != save_errno)
	{
		gtm_putmsg(VARLSTCNT(5) ERR_DBFILOPERR, 2, LEN_AND_STR(fn), save_errno);
 		CLOSEFILE(fd, save_errno);
		return FALSE;
	}
	if (memcmp(header->label, GDS_LABEL, GDS_LABEL_SZ - 1))
	{
		gtm_putmsg(VARLSTCNT(4) ERR_DBNOTGDS, 2, LEN_AND_STR(fn));
 		CLOSEFILE(fd, save_errno);
		return FALSE;
	}
	CLOSEFILE(fd, save_errno);
	if (0 != save_errno)
	{
		gtm_putmsg(VARLSTCNT(5) ERR_DBFILOPERR, 2, LEN_AND_STR(fn), save_errno);
		return FALSE;
	}
	return TRUE;
}
