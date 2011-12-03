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
#include "gtm_time.h"
#include "gtm_unistd.h"
#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "jnl.h"
#include "send_msg.h"
#include "repl_sp.h"
#include "iosp.h"	/* for SS_NORMAL */

GBLREF 	jnl_gbls_t		jgbl;

error_def(ERR_JNLFILOPN);
error_def(ERR_JNLINVALID);
error_def(ERR_PREVJNLLINKCUT);

uint4 jnl_file_open_switch(gd_region *reg, uint4 sts)
{
	sgmnt_addrs		*csa;
	jnl_private_control	*jpc;
	jnl_create_info		create;
	char			prev_jnl_fn[JNL_NAME_SIZE];

	csa = &FILE_INFO(reg)->s_addrs;
	jpc = csa->jnl;

	assert((ERR_JNLFILOPN != sts) && (NOJNL != jpc->channel) || (ERR_JNLFILOPN == sts) && (NOJNL == jpc->channel));
	if ((ERR_JNLFILOPN != sts) && (NOJNL != jpc->channel))
		F_CLOSE(jpc->channel);
	jpc->channel = NOJNL;
	jnl_send_oper(jpc, sts);
	/* attempt to create a new journal file */
	memset(&create, 0, sizeof(create));
	create.status = create.status2 = SS_NORMAL;
	create.prev_jnl = &prev_jnl_fn[0];
	set_jnl_info(reg, &create);
	create.no_prev_link = TRUE;
	create.no_rename = FALSE;
	if (!jgbl.forw_phase_recovery)
		JNL_SHORT_TIME(jgbl.gbl_jrec_time);	/* needed for cre_jnl_file() */
	/* else mur_output_record() would have already set jgbl.gbl_jrec_time */
	assert(jgbl.gbl_jrec_time);
	if (EXIT_NRM != cre_jnl_file(&create))
	{
		jpc->status = create.status;
		jpc->status2 = create.status2;
		return ERR_JNLINVALID;
	} else
	{
		jpc->status = SS_NORMAL;
		sts = 0;
	}
	send_msg(VARLSTCNT(6) ERR_PREVJNLLINKCUT, 4, JNL_LEN_STR(csa->hdr), DB_LEN_STR(reg));
	assert(csa->hdr->jnl_file_len == create.jnl_len);
	assert(0 == memcmp(csa->hdr->jnl_file_name, create.jnl, create.jnl_len));
	return sts;
}
