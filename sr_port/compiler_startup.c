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

#include "compiler.h"
#include "opcode.h"
#include "cmd_qlf.h"
#include "mdq.h"
#include "cgp.h"
#include "error.h"
#include "mmemory.h"
#include "stringpool.h"
#include "list_file.h"
#include "source_file.h"
#include "lb_init.h"
#include "reinit_externs.h"
#include "comp_esc.h"
#include "resolve_blocks.h"

#define HOPELESS_COMPILE 128

GBLREF short int source_column, source_line;

/* ensure source_buffer is aligned on a int4 word boundary so that
 * we can calculate the checksum a longword at a time.
 */
GBLREF int4 aligned_source_buffer[MAX_SRCLINE / sizeof(int4) + 1];
GBLREF unsigned char *source_buffer;
GBLREF int4 source_error_found;
GBLREF src_line_struct src_head;
GBLREF bool code_generated;

GBLREF triple			t_orig, *curr_fetch_trip, *curr_fetch_opr;
GBLREF int4			curr_fetch_count;
GBLREF command_qualifier	cmd_qlf;
GBLREF int			mlmax;
GBLREF mline			mline_root;
GBLREF char			cg_phase;	/* code generation phase */

bool compiler_startup(void)
{
#ifdef DEBUG
	void	dumpall();
#endif
	bool	compile_w_err;
	unsigned char	err_buf[45];
	unsigned char *cp, *cp2;
	int	errknt, n;
	uint4	checksum, srcint, line_count;
	mlabel	*null_lab;
	src_line_struct	*sl;
	mident	null_mident;
	static readonly char compile_terminated[] = "COMPILATION TERMINATED DUE TO EXCESS ERRORS";

	memset(&null_mident, 0, sizeof(null_mident));
	ESTABLISH_RET(compiler_ch, FALSE);
	cg_phase = CGP_NOSTATE;
	source_error_found = errknt = 0;
	if(!open_source_file())
	{
		REVERT;
		return FALSE;
	}
	cg_phase = CGP_PARSE;
	if (cmd_qlf.qlf & CQ_LIST || cmd_qlf.qlf & CQ_CROSS_REFERENCE)
	{
		if (cmd_qlf.qlf &  CQ_MACHINE_CODE)
			dqinit(&src_head, que);
		open_list_file();
	}
	if (cmd_qlf.qlf & CQ_CE_PREPROCESS)
		open_ceprep_file();
	tripinit();
	null_lab = get_mladdr(&null_mident);
	null_lab->ml = &mline_root;
	mlmax++;
	curr_fetch_trip = curr_fetch_opr = newtriple(OC_LINEFETCH);
	curr_fetch_count = 0;
	code_generated = FALSE;
	checksum = 0;
	line_count = 1;
	for (source_line = 1;  errknt <= HOPELESS_COMPILE;  source_line++)
	{
		if (-1 == (n = read_source_file()))
			break;
		if (cmd_qlf.qlf & CQ_LIST || cmd_qlf.qlf & CQ_CROSS_REFERENCE)
		{
			if (cmd_qlf.qlf & CQ_MACHINE_CODE)
			{
				sl = (src_line_struct *)mcalloc(sizeof(src_line_struct));
				dqins(&src_head, que, sl);
				sl->addr = mcalloc(n + 1);	/* +1 for zero termination */
				sl->line = source_line;
				memcpy(sl->addr, source_buffer, n + 1);
			} else
			{
				list_line_number();
				list_line((char *)source_buffer);
			}
		}
		/* calculate checksum */
		for (cp = source_buffer, cp2 = cp + n;  cp < cp2;)
		{
			srcint = 0;
			if (cp2 - cp < sizeof(int4))
			{
				memcpy(&srcint, cp, cp2 - cp);
				cp = cp2;
			} else
			{
				srcint = *(int4 *)cp;
				cp += sizeof(int4);
			}
			checksum ^= srcint;
			checksum >>= 1;
		}
		source_error_found = 0;
		lb_init();
		if (cmd_qlf.qlf & CQ_CE_PREPROCESS)
			put_ceprep_line();
		if (!line(&line_count))
		{
			assert(source_error_found);
			errknt++;
		}
	}
	close_source_file();
	if (cmd_qlf.qlf & CQ_CE_PREPROCESS)
		close_ceprep_file();
	cg_phase = CGP_RESOLVE;
	if (t_orig.exorder.fl == &t_orig)	/* if no lines in routine, set up line 0 */
		newtriple(OC_LINESTART);
	newtriple(OC_RET);			/* always provide a default QUIT */
	mline_root.externalentry = t_orig.exorder.fl;
	stp_gcol(0);
	start_fetches(OC_NOOP);
	resolve_blocks();
	errknt = resolve_ref(errknt);
	compile_w_err = (errknt <= HOPELESS_COMPILE && (cmd_qlf.qlf & CQ_IGNORE));
	if (cmd_qlf.qlf & CQ_LIST || cmd_qlf.qlf & CQ_CROSS_REFERENCE)
	{
		list_line("");
		if (errknt)
			cp = i2asc(err_buf, errknt);
		else
		{
			cp = err_buf;
			*cp++ = 'n';
			*cp++ = 'o';
		}
		memcpy(cp, " error", sizeof(" error"));
		cp += sizeof(" error") - 1;
		if (1 != errknt)
			*cp++ = 's';
		*cp = 0;
		list_line((char *)err_buf);
		if (errknt > HOPELESS_COMPILE)
			list_line(compile_terminated);
		if (cmd_qlf.qlf & CQ_MACHINE_CODE && compile_w_err)
			list_head(1);
	}
	if ((!errknt || compile_w_err) && (cmd_qlf.qlf & CQ_OBJECT || cmd_qlf.qlf & CQ_MACHINE_CODE))
	{
		obj_code(line_count, checksum);
		cg_phase = CGP_FINI;
	}
	if (cmd_qlf.qlf & CQ_LIST || cmd_qlf.qlf & CQ_CROSS_REFERENCE)
	{
		list_cmd();
		close_list_file();
	}
	reinit_externs();
	REVERT;
	if (errknt)
		return TRUE;
	else
		return FALSE;
}
