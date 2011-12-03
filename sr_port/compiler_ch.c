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

#include "error.h"
#include "cgp.h"
#include "cmd_qlf.h"
#include "list_file.h"
#include "source_file.h"
#include "rtnhdr.h"
#include "obj_file.h"
#include "reinit_externs.h"
#include "compiler.h"
#include "util.h"

GBLREF command_qualifier	cmd_qlf;
GBLREF char			cg_phase;

CONDITION_HANDLER(compiler_ch)
{
	error_def(ERR_ASSERT);
	error_def(ERR_FORCEDHALT);
	error_def(ERR_GTMASSERT);
	error_def(ERR_GTMCHECK);
	error_def(ERR_STACKOFLOW);

	START_CH;
	if (DUMP)
	{
		NEXTCH;
	}

	if (cmd_qlf.qlf & CQ_WARNINGS)
		PRN_ERROR;

	reinit_externs();

	if (cg_phase == CGP_MACHINE)
		drop_object_file();

	if (cg_phase > CGP_NOSTATE)
	{
		if (cg_phase < CGP_RESOLVE)
			close_source_file();
		if (cg_phase < CGP_FINI  &&  (cmd_qlf.qlf & CQ_LIST  ||  cmd_qlf.qlf & CQ_CROSS_REFERENCE))
		{
			close_list_file();
		}
	}
	UNWIND(NULL, NULL);
}
