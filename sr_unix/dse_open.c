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

#include "gtm_string.h"
#include "gtm_fcntl.h"

#include <unistd.h>
#include <errno.h>
#include "gtm_stat.h"
#include "gtm_iconv.h"

#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "gdsblk.h"
#include "cli.h"
#include "dse.h"
#include "gtmio.h"
#include "io.h"
#include "iosp.h"
#include "io_params.h"
#include "stringpool.h"
#include "util.h"
#include "op.h"

GBLREF int	sys_nerr;

#ifdef	__osf__
#pragma pointer_size (save)
#pragma pointer_size (long)
#endif

GBLREF int	(*op_open_ptr)(mval *v, mval *p, int t, mval *mspace);
GBLREF spdesc stringpool;
GBLREF iconv_t          dse_over_cvtcd;

#ifdef	__osf__
#pragma pointer_size (restore)
#endif

static int	patch_fd;
static char	patch_ofile[256];
static short	patch_len;
static char     ch_set_name[MAX_CHSET_NAME];
static short    ch_set_len;

GBLREF enum dse_fmt	dse_dmp_format;

void	dse_open (void)
{
	unsigned short	cli_len;
	int4 	save_errno;

	mval            val;
	mval            pars;
	int		cnt;
	static readonly unsigned char open_params_list[2] =
	{
		(unsigned char)iop_newversion,
		(unsigned char)iop_eol
	};

	if (cli_present("FILE") == CLI_PRESENT)
	{
		if (CLOSED_FMT != dse_dmp_format)
		{
			util_out_print("Error:  output file already open.",TRUE);
			util_out_print("Current output file:  !AD", TRUE, strlen(patch_ofile), &patch_ofile[0]);
			return;
		}
		cli_len = sizeof(patch_ofile);
		if (!cli_get_str("FILE", patch_ofile, &cli_len))
			return;
		if (0 == cli_len)
		{
			util_out_print("Error: must specify a file name.",TRUE);
			return;
		}
		patch_ofile[cli_len] = 0;
		patch_len = cli_len;

		pars.mvtype = MV_STR;
		pars.str.len = sizeof(open_params_list);
		pars.str.addr = (char *)open_params_list;
		val.mvtype = MV_STR;
		val.str.len = patch_len;
		val.str.addr = (char *)patch_ofile;
		(*op_open_ptr)(&val, &pars, 0, NULL);
		op_use(&val, &pars);

		if (CLI_PRESENT == cli_present("OCHSET"))
		{
			if (cli_get_str("OCHSET", ch_set_name, &cli_len))
			{
				if (0 == cli_len)
				{
					util_out_print("Error: must specify a charactor set name.",TRUE);
					return;
				}
				ch_set_name[cli_len] = 0;
				ch_set_len = cli_len;
				if ( (iconv_t)0 != dse_over_cvtcd )
				{
					ICONV_CLOSE_CD(dse_over_cvtcd);
				}
				ICONV_OPEN_CD(dse_over_cvtcd, INSIDE_CH_SET, ch_set_name);
			}
		} else	if ( (iconv_t) 0 == dse_over_cvtcd )
				ICONV_OPEN_CD(dse_over_cvtcd, INSIDE_CH_SET, OUTSIDE_CH_SET);

		dse_dmp_format = OPEN_FMT;
	} else
	{
		if (CLOSED_FMT != dse_dmp_format)
			util_out_print("Current output file:  !AD", TRUE, strlen(patch_ofile), &patch_ofile[0]);
		else
			util_out_print("No current output file.",TRUE);
	}
	return;

}

boolean_t dse_fdmp_output (void *addr, int4 len)
{
	mval		val;
	static char	*buffer = NULL;
	static int	bufsiz = 0;

	assert(len >= 0);
	if (len + 1 > bufsiz)
	{
		if (buffer)
			free(buffer);
		bufsiz = len + 1;
		buffer = (char *)malloc(bufsiz);
	}
	if (len)
	{
		memcpy(buffer, addr, len);
		buffer[len] = 0;
		val.mvtype = MV_STR;
		val.str.addr = (char *)buffer;
		val.str.len = len;
		op_write(&val);
	}
	op_wteol(1);
	return TRUE;
}

void	dse_close(void)
{
	mval            val;
	mval            pars;
	unsigned char   no_param = (unsigned char)iop_eol;

	if (CLOSED_FMT != dse_dmp_format)
	{
		util_out_print("Closing output file:  !AD",TRUE,LEN_AND_STR(patch_ofile));
		val.mvtype = pars.mvtype = MV_STR;
		val.str.addr = (char *)patch_ofile;
		val.str.len = patch_len;
		pars.str.len = sizeof(iop_eol);
		pars.str.addr = (char *)&no_param;
		op_close(&val, &pars);
		dse_dmp_format = CLOSED_FMT;
	}
	else
		util_out_print("Error:  no current output file.",TRUE);
	return;
}
