/****************************************************************
 *								*
 *	Copyright 2002, 2003 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "gtm_stdio.h"
#include "gtm_string.h"
#include "cli.h"
#include "cli_parse.h"
#include "cli_disallow.h"
#include "mupip_cmd_disallow.h"

GBLREF	char	*cli_err_str_ptr;

boolean_t cli_disallow_mupip_backup(void)
{
	int disallow_return_value = 0;

	*cli_err_str_ptr = 0;
	disallow_return_value =  (d_c_cli_present("TRANSACTION") || d_c_cli_present("SINCE"))
					&& !(d_c_cli_present("INCREMENTAL") || d_c_cli_present("BYTESTREAM"));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  (d_c_cli_present("INCREMENTAL") || d_c_cli_present("BYTESTREAM"))
					&& (d_c_cli_present("COMPREHENSIVE") || d_c_cli_present("DATABASE"));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value = d_c_cli_present("TRANSACTION") && d_c_cli_present("SINCE");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value = d_c_cli_present("BKUPDBJNL.DISABLE") && d_c_cli_present("BKUPDBJNL.OFF");
	CLI_DIS_CHECK_N_RESET;
	return FALSE;
}

boolean_t cli_disallow_mupip_freeze(void)
{
	int disallow_return_value = 0;

	*cli_err_str_ptr = 0;
	disallow_return_value = !d_c_cli_present("ON") && !d_c_cli_present("OFF");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value = d_c_cli_present("RECORD") && !d_c_cli_present("ON");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value = d_c_cli_present("OVERRIDE") && !d_c_cli_present("OFF");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value = d_c_cli_present("ON") && d_c_cli_present("OFF");
	CLI_DIS_CHECK_N_RESET;
	return FALSE;
}

boolean_t cli_disallow_mupip_integ(void)
{
	int disallow_return_value = 0;

	*cli_err_str_ptr = 0;
	disallow_return_value = d_c_cli_present("BRIEF") && d_c_cli_present("FULL");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value = d_c_cli_present("FILE") && d_c_cli_present("REGION");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value = d_c_cli_present("TN_RESET") && (d_c_cli_present("FAST")
								|| d_c_cli_present("BLOCK")
								|| d_c_cli_present("SUBSCRIPT")
								|| d_c_cli_present("REGION"));
	CLI_DIS_CHECK_N_RESET;
	return FALSE;
}

boolean_t cli_disallow_mupip_journal(void)
{
	int disallow_return_value = 0;

	*cli_err_str_ptr = 0;
	disallow_return_value =  !(d_c_cli_present("RECOVER")
					|| d_c_cli_present("VERIFY")
					|| d_c_cli_present("SHOW")
					|| d_c_cli_present("EXTRACT")
					|| d_c_cli_present("ROLLBACK"));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("RECOVER") && d_c_cli_present("ROLLBACK");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  !(d_c_cli_present("FORWARD") || d_c_cli_present("BACKWARD"));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("FORWARD") && d_c_cli_present("BACKWARD");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("SINCE") && d_c_cli_present("FORWARD");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("LOOKBACK_LIMIT") && d_c_cli_present("FORWARD");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("REDIRECT") && !d_c_cli_present("RECOVER");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("CHECKTN") && d_c_cli_present("BACKWARD");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("RESYNC") && d_c_cli_present("FETCHRESYNC");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  (d_c_cli_present("RESYNC") || d_c_cli_present("FETCHRESYNC"))
					&& !d_c_cli_present("ROLLBACK");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("LOSTTRANS") && !(d_c_cli_present("RECOVER")
									|| d_c_cli_present("ROLLBACK")
									|| d_c_cli_present("EXTRACT"));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("BROKENTRANS") && !(d_c_cli_present("RECOVER")
									|| d_c_cli_present("ROLLBACK")
									|| d_c_cli_present("EXTRACT"));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("FORWARD") && d_c_cli_present("ROLLBACK");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("FULL") && (d_c_cli_present("RECOVER") || d_c_cli_present("ROLLBACK"));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("DETAIL") && !d_c_cli_present("EXTRACT");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("AFTER") && !d_c_cli_present("FORWARD");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("AFTER") && (d_c_cli_present("RECOVER") || d_c_cli_present("ROLLBACK"));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("SINCE") && !d_c_cli_present("BACKWARD");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("LOOKBACK_LIMIT") && !d_c_cli_present("BACKWARD");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("LOOKBACK_LIMIT") && !(d_c_cli_present("VERIFY")
									|| d_c_cli_present("RECOVER")
									|| d_c_cli_present("EXTRACT")
									|| d_c_cli_present("SHOW"));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("APPLY_AFTER_IMAGE") && !(d_c_cli_present("ROLLBACK")
										|| d_c_cli_present("RECOVER"));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("REDIRECT") && !d_c_cli_present("RECOVER");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("REDIRECT") && !d_c_cli_present("FORWARD");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("BACKWARD") && d_c_cli_negated("CHAIN");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("CHECKTN") && d_c_cli_present("BACKWARD");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  d_c_cli_present("ROLLBACK") && (d_c_cli_present("AFTER")
									|| d_c_cli_present("BEFORE")
									|| d_c_cli_present("SINCE")
									|| d_c_cli_present("LOOKBACK_LIMIT"));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  (d_c_cli_present("GLOBAL")
					|| d_c_cli_present("USER")
					|| d_c_cli_present("ID")
					|| d_c_cli_present("TRANSACTION")) && (d_c_cli_present("RECOVER")
										|| d_c_cli_present("ROLLBACK")
										|| d_c_cli_present("VERIFY"));
	CLI_DIS_CHECK_N_RESET;
	return FALSE;
}

boolean_t cli_disallow_mupip_replicate(void)
{
	int disallow_return_value = 0;

	*cli_err_str_ptr = 0;
	disallow_return_value = !(d_c_cli_present("RECEIVER")
					|| d_c_cli_present("SOURCE")
					|| d_c_cli_present("UPDATEPROC")
					|| d_c_cli_present("INSTANCE_CREATE"));
	CLI_DIS_CHECK_N_RESET;
	return FALSE;
}

boolean_t cli_disallow_mupip_replic_receive(void)
{
	int		disallow_return_value = 0;
	boolean_t	p1, p2, p3, p4, p5, p6;

	*cli_err_str_ptr = 0;

	p1 = d_c_cli_present("START");
	p2 = d_c_cli_present("SHUTDOWN");
	p3 = d_c_cli_present("CHECKHEALTH");
	p4 = d_c_cli_present("STATSLOG");
	p5 = d_c_cli_present("SHOWBACKLOG");
	p6 = d_c_cli_present("CHANGELOG");
	disallow_return_value = cli_check_any2(VARLSTCNT(6) p1, p2, p3, p4, p5, p6);
	CLI_DIS_CHECK_N_RESET;

	disallow_return_value = !(d_c_cli_present("START")
					|| d_c_cli_present("SHUTDOWN")
					|| d_c_cli_present("CHECKHEALTH")
					|| d_c_cli_present("STATSLOG")
					|| d_c_cli_present("SHOWBACKLOG")
					|| d_c_cli_present("CHANGELOG"));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value = ((d_c_cli_present("START") && !d_c_cli_present("UPDATEONLY")) || d_c_cli_present("CHANGELOG"))
					&& !d_c_cli_present("LOG");
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value = !d_c_cli_present("START") && d_c_cli_present("UPDATERESYNC");
	CLI_DIS_CHECK_N_RESET;
	return FALSE;
}

boolean_t cli_disallow_mupip_replic_source(void)
{
	int		disallow_return_value = 0;
	boolean_t	p1, p2, p3, p4, p5, p6, p7, p8, p9;

	*cli_err_str_ptr = 0;

	p1 = d_c_cli_present("START");
	p2 = d_c_cli_present("SHUTDOWN");
	p3 = d_c_cli_present("ACTIVATE");
	p4 = d_c_cli_present("DEACTIVATE");
	p5 = d_c_cli_present("CHECKHEALTH");
	p6 = d_c_cli_present("STATSLOG");
	p7 = d_c_cli_present("SHOWBACKLOG");
	p8 = d_c_cli_present("CHANGELOG");
	p9 = d_c_cli_present("STOPSOURCEFILTER");
	disallow_return_value = cli_check_any2(VARLSTCNT(9) p1, p2, p3, p4, p5, p6, p7, p8, p9);
	CLI_DIS_CHECK_N_RESET;

	disallow_return_value = !(d_c_cli_present("UPDATE")
					|| d_c_cli_present("START")
					|| d_c_cli_present("SHUTDOWN")
					|| d_c_cli_present("ACTIVATE")
					|| d_c_cli_present("DEACTIVATE")
					|| d_c_cli_present("CHECKHEALTH")
					|| d_c_cli_present("STATSLOG")
					|| d_c_cli_present("SHOWBACKLOG")
					|| d_c_cli_present("CHANGELOG")
					|| d_c_cli_present("STOPSOURCEFILTER"));
	CLI_DIS_CHECK_N_RESET;
	return FALSE;
}

boolean_t cli_disallow_mupip_rundown(void)
{
	int disallow_return_value = 0;

	*cli_err_str_ptr = 0;
	disallow_return_value = d_c_cli_present("FILE") && d_c_cli_present("REGION");
	CLI_DIS_CHECK_N_RESET;
	return FALSE;
}

boolean_t cli_disallow_mupip_set(void)
{
	int		disallow_return_value = 0;
	boolean_t	p1, p2, p3;

	*cli_err_str_ptr = 0;

	p1 = d_c_cli_present("FILE");
	p2 = d_c_cli_present("REGION");
	p3 = d_c_cli_present("JNLFILE");
	disallow_return_value =  cli_check_any2(VARLSTCNT(3) p1, p2, p3);
	CLI_DIS_CHECK_N_RESET;

	disallow_return_value =  (!(d_c_cli_present("FILE") || d_c_cli_present("REGION") || d_c_cli_present("JNLFILE")));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  (d_c_cli_present("JOURNAL.ON") && d_c_cli_present("JOURNAL.OFF"));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  (d_c_cli_present("JOURNAL.DISABLE") &&
		(d_c_cli_present("JOURNAL.ON") 	|| d_c_cli_present("JOURNAL.OFF")           ||
		  d_c_cli_present("JOURNAL.ENABLE") 	||
		  d_c_cli_present("JOURNAL.BEFORE_IMAGES") || d_c_cli_negated("JOURNAL.BEFORE_IMAGES") ||
		  d_c_cli_present("JOURNAL.FILENAME") 	|| d_c_cli_present("JOURNAL.ALLOCATION")    ||
		  d_c_cli_present("JOURNAL.EXTENSION")	|| d_c_cli_present("JOURNAL.BUFFER_SIZE")   ||
		  d_c_cli_present("JOURNAL.ALIGNSIZE") 	|| d_c_cli_present("JOURNAL.EPOCH_INTERVAL") ||
		  d_c_cli_present("JOURNAL.AUTOSWITCHLIMIT")));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value = (!(!d_c_cli_present("JOURNAL") || d_c_cli_present("DISABLE") || d_c_cli_present("OFF") ||
		d_c_cli_present("JOURNAL.BEFORE_IMAGES") || d_c_cli_negated("JOURNAL.BEFORE_IMAGES")));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value = (d_c_cli_present("REPLICATION.ON") && d_c_cli_present("REPLICATION.OFF"));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value = (d_c_cli_present("REPLICATION.ON") && (d_c_cli_present("JOURNAL.OFF") ||
		d_c_cli_present("JOURNAL.DISABLE") || d_c_cli_negated("JOURNAL") || d_c_cli_negated("JOURNAL.BEFORE_IMAGES")));
	CLI_DIS_CHECK_N_RESET;
	disallow_return_value =  (d_c_cli_present("PREVJNLFILE") && !(d_c_cli_present("JNLFILE")));
	CLI_DIS_CHECK_N_RESET;
	return FALSE;
}

