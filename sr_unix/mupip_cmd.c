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
#include "gdsroot.h"
#include "gdskill.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "repl_msg.h"
#include "cli.h"
#include "mupip_cmd_disallow.h"

/*********************************************************************
 * Parameters must be defined in the order they are to be specified
 * AND must be alphabetical
 *
 * This file might have lines longer than 132 characters
 * since the command tables are being initialized.
 *********************************************************************/

#include "mupip_backup.h"
#include "mupip_create.h"
#include "mupip_cvtgbl.h"
#include "mupip_cvtpgm.h"
#include "mupip_extend.h"
#include "muextr.h"
#include "mupip_freeze.h"
#include "mupip_help.h"
#include "mupip_integ.h"
#include "mupip_intrpt.h"
#include "mupip_quit.h"
#include "mupip_recover.h"
#include "mupip_reorg.h"
#include "mupip_restore.h"
#include "mupip_rundown.h"
#include "mupip_set.h"
#include "mupip_stop.h"
#include "mupip_upgrade.h"
#include "mupip_ftok.h"

#include "gtmsource.h"
#include "gtmrecv.h"
#include "updproc.h"
#include "repl_instance.h"

static CLI_ENTRY mup_set_journal_qual[] = {
	{ "ALIGNSIZE",       0, 0, 0, 0, 0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM, 0 },
	{ "ALLOCATION",      0, 0, 0, 0, 0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM, 0 },
	{ "AUTOSWITCHLIMIT", 0, 0, 0, 0, 0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM, 0 },
	{ "BEFORE_IMAGES",   0, 0, 0, 0, 0, 0, VAL_DISALLOWED, 0, NEG,     VAL_N_A, 0 },
	{ "BUFFER_SIZE",     0, 0, 0, 0, 0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM, 0 },
	{ "DISABLE",         0, 0, 0, 0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{ "ENABLE",          0, 0, 0, 0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{ "EPOCH_INTERVAL",  0, 0, 0, 0, 0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM, 0 },
	{ "EXTENSION",       0, 0, 0, 0, 0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM, 0 },
	{ "FILENAME",        0, 0, 0, 0, 0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
	{ "OFF",             0, 0, 0, 0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{ "ON",              0, 0, 0, 0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{ "SYNC_IO",         0, 0, 0, 0, 0, 0, VAL_DISALLOWED, 0, NEG,     VAL_N_A, 0 },
	{ "YIELD_LIMIT",     0, 0, 0, 0, 0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM, 0 },
	{ 0 }
};

static CLI_ENTRY mub_since_qual[] = {
	{ "BYTESTREAM",		0, 0, 0, 0, 0, 0,            VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ "COMPREHENSIVE",	0, 0, 0, 0, 0, 0,            VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ "DATABASE",		0, 0, 0, 0, 0, DEFA_PRESENT, VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ "INCREMENTAL",	0, 0, 0, 0, 0, 0,            VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ "RECORD",		0, 0, 0, 0, 0, 0,            VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ 0 }
};

static CLI_ENTRY mup_set_acc_qual[] = {
	{ "BG",	0, 0, 0, 0, 0, 0, VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ "MM",	0, 0, 0, 0, 0, 0, VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ 0 }
};

static CLI_ENTRY mup_extract_format_qual[] = {
	{ "BINARY",	0, 0, 0, 0, 0, 0,            VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ "GO",		0, 0, 0, 0, 0, 0,            VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ "ZWR",	0, 0, 0, 0, 0, DEFA_PRESENT, VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ 0 }
};

static CLI_ENTRY mup_convert_format_qual[] = {
	{ "RO",	0, 0, 0, 0, 0, 0, VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ 0 }
};

static CLI_ENTRY mup_jnl_show_qual[] = {
	{ "ACTIVE_PROCESSES",	 0, 0, 0, 0, 0, 0,            VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ "ALL",		 0, 0, 0, 0, 0, DEFA_PRESENT, VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ "BROKEN_TRANSACTIONS", 0, 0, 0, 0, 0, 0,            VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ "HEADER",		 0, 0, 0, 0, 0, 0,            VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ "PROCESSES",		 0, 0, 0, 0, 0, 0,            VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ "STATISTICS",		 0, 0, 0, 0, 0, 0,            VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ 0 }
};

static CLI_ENTRY mup_load_fmt_qual[] = {
	{ "BINARY",	0, 0, 0, 0, 0, 0,            VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ "GO",		0, 0, 0, 0, 0, 0,            VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ "ZWR",	0, 0, 0, 0, 0, DEFA_PRESENT, VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ 0 }
};

static CLI_ENTRY mup_jnl_fences_qual[] = {
	{ "ALWAYS",	0, 0, 0, 0, 0, 0,            VAL_DISALLOWED,	0,	NON_NEG,	VAL_N_A,	0 },
	{ "NONE",	0, 0, 0, 0, 0, 0,            VAL_DISALLOWED,	0,	NON_NEG,	VAL_N_A,	0 },
	{ "PROCESS",	0, 0, 0, 0, 0, DEFA_PRESENT, VAL_DISALLOWED,	0,	NON_NEG,	VAL_N_A,	0 },
	{ 0 }
};

static CLI_ENTRY mup_set_repl_qual[] = {
	{ "OFF",	0, 0, 0, 0, 0, 0, VAL_DISALLOWED,	0,	NON_NEG,	VAL_N_A,	0 },
	{ "ON",		0, 0, 0, 0, 0, 0, VAL_DISALLOWED,	0,	NON_NEG,	VAL_N_A,	0 },
	{ 0 }
};

static	CLI_PARM	mup_backup_parm[] = {
	{ "REG_NAME", "Region: " },
	{ "SAVE_DIR", "Backup Directory: " },
	{ "", "" }
};

static CLI_ENTRY mub_njnl_val_qual[] = {
	{ "PREVLINK",	0, 0, 0, 0, 0, 0, VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ "SYNC_IO",	0, 0, 0, 0, 0, 0, VAL_DISALLOWED,	0,	NEG,	VAL_N_A,	0 },
	{ 0 }
};

static CLI_ENTRY mub_dbjnl_val_qual[] = {
	{ "DISABLE",	0, 0, 0, 0, 0, 0, VAL_DISALLOWED,	1,	NEG,	VAL_N_A,	0 },
	{ "OFF",	0, 0, 0, 0, 0, 0, VAL_DISALLOWED,	1,	NEG,	VAL_N_A,	0 },
	{ 0 }
};

static readonly CLI_PARM mub_njnl_parm[] = {
	{ "NEWJNLFILES", "PREVLINK"}
};

static	CLI_ENTRY	mup_backup_qual[] = {
	{ "BKUPDBJNL",     mupip_backup, 0, 0,             mub_dbjnl_val_qual, 0, 0, VAL_REQ,        2, NON_NEG, VAL_STR, 0       },
	{ "BYTESTREAM",    mupip_backup, 0, 0,             0,                  0, 0, VAL_DISALLOWED, 2, NON_NEG, VAL_N_A, 0       },
	{ "COMPREHENSIVE", mupip_backup, 0, 0,             0,                  0, 0, VAL_DISALLOWED, 2, NON_NEG, VAL_N_A, 0       },
	{ "DATABASE",      mupip_backup, 0, 0,             0,                  0, 0, VAL_DISALLOWED, 2, NON_NEG, VAL_N_A, 0       },
	{ "DBG",           mupip_backup, 0, 0,             0,                  0, 0, VAL_DISALLOWED, 2, NON_NEG, VAL_N_A, 0       },
	{ "INCREMENTAL",   mupip_backup, 0, 0,             0,                  0, 0, VAL_DISALLOWED, 2, NON_NEG, VAL_N_A, 0       },
	{ "JOURNAL",       mupip_backup, 0, 0,             0,                  0, 0, VAL_REQ,        2, NEG,     VAL_STR, 0       },
	{ "NETTIMEOUT",    mupip_backup, 0, 0,             0,                  0, 0, VAL_REQ,        2, NON_NEG, VAL_NUM, 0       },
	{ "NEWJNLFILES",   mupip_backup, 0, mub_njnl_parm, mub_njnl_val_qual,  0, 0, VAL_NOT_REQ,    2, NEG,     VAL_STR, 0       },
	{ "ONLINE",        mupip_backup, 0, 0,             0,                  0, 0, VAL_DISALLOWED, 2, NEG,     VAL_N_A, 0       },
	{ "RECORD",        mupip_backup, 0, 0,             0,                  0, 0, VAL_DISALLOWED, 2, NON_NEG, VAL_N_A, 0       },
	{ "REPLACE",       mupip_backup, 0, 0,             0,                  0, 0, VAL_DISALLOWED, 2, NON_NEG, VAL_N_A, 0       },
	{ "SINCE",         mupip_backup, 0, 0,             mub_since_qual,     0, 0, VAL_REQ,        2, NON_NEG, VAL_STR, 0       },
	{ "TRANSACTION",   mupip_backup, 0, 0,             0,                  0, 0, VAL_REQ,        2, NON_NEG, VAL_NUM, VAL_HEX },
	{ 0 }
};

static	CLI_PARM	mup_convert_parm[] = {
	{ "FILE", "Input File: " },
	{ "", "" }
};

static	CLI_ENTRY	mup_convert_qual[] = {
	{ "FORMAT", mupip_cvtpgm, 0, 0, mup_convert_format_qual, 0, 0, VAL_REQ, 2, NON_NEG, VAL_STR, 0 },
	{ 0 }
};

static	CLI_ENTRY	mup_create_qual[] = {
	{ "REGION", mupip_create, 0, 0, 0, 0, 0, VAL_REQ, 0, NON_NEG, VAL_STR, 0 },
	{ 0 }
};

static	CLI_PARM	mup_extend_parm[] = {
	{ "REG_NAME", "Region: " },
	{ "", "" }
};

static	CLI_ENTRY	mup_extend_qual[] = {
	{ "BLOCKS", mupip_extend, 0, 0, 0, 0, 0, VAL_REQ, 1, NON_NEG, VAL_NUM, 0 },
	{ 0 }
};

static	CLI_PARM	mup_extract_parm[] = {
	{ "FILE", "Output File: " },
	{ "", "" }
};

static readonly CLI_PARM mup_extr_label_parm[] = {
	{ "LABEL", "GT.M MUPIP EXTRACT"}
};

static	CLI_ENTRY	mup_extract_qual[] = {
	{ "FORMAT", mu_extract, 0, 0,                   mup_extract_format_qual, 0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
	{ "FREEZE", mu_extract, 0, 0,                   0,                       0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0 },
	{ "LABEL",  mu_extract, 0, mup_extr_label_parm, 0,                       0, 0, VAL_NOT_REQ,    1, NON_NEG, VAL_STR, 0 },
	{ "LOG",    mu_extract, 0, 0,                   0,                       0, 0, VAL_DISALLOWED, 1, NEG,     VAL_N_A, 0 },
	{ "SELECT", mu_extract, 0, 0,                   0,                       0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
	{ "OCHSET", mu_extract, 0, 0,                   0,                       0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
	{ 0 }
};

static	CLI_PARM	mup_freeze_parm[] = {
	{ "REG_NAME", "Region: " },
	{ "", "" }
};

static	CLI_ENTRY	mup_freeze_qual[] = {
	{ "OFF",      mupip_freeze, 0, 0, 0, 0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0 },
	{ "ON",       mupip_freeze, 0, 0, 0, 0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0 },
	{ "OVERRIDE", mupip_freeze, 0, 0, 0, 0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0 },
	{ "RECORD",   mupip_freeze, 0, 0, 0, 0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0 },
	{ 0 }
};
static	CLI_PARM	mup_ftok_parm[] = {
	{ "FILE", "File: " },
	{ "", "" }
};

static	CLI_ENTRY	mup_ftok_qual[] = {
        { "DB",		mupip_ftok, 0, 0, 0, 0, 0, VAL_DISALLOWED,		1, NON_NEG, VAL_N_A, 0 },
        { "JNLPOOL",	mupip_ftok, 0, 0, 0, 0, 0, VAL_DISALLOWED,		1, NON_NEG, VAL_N_A, 0 },
        { "RECVPOOL",	mupip_ftok, 0, 0, 0, 0, 0, VAL_DISALLOWED,		1, NON_NEG, VAL_N_A, 0 },
	{ 0 }
};

static	CLI_PARM	mup_integ_parm[] = {
	{ "WHAT", "File or Region: " },
	{ "", "" }
};

static readonly CLI_PARM mup_integ_map_parm[] = {
	{ "MAP", "10"}
};

static	CLI_ENTRY	mup_integ_qual[] = {
	{ "ADJACENCY",   mupip_integ, 0, 0,                  0, 0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM, 0       },
	{ "BLOCK",       mupip_integ, 0, 0,                  0, 0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM, VAL_HEX },
	{ "BRIEF",       mupip_integ, 0, 0,                  0, 0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0       },
	{ "FAST",        mupip_integ, 0, 0,                  0, 0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0       },
	{ "FILE",        mupip_integ, 0, 0,                  0, 0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0       },
	{ "FULL",        mupip_integ, 0, 0,                  0, 0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0       },
	{ "KEYRANGES",   mupip_integ, 0, 0,                  0, 0, 0, VAL_DISALLOWED, 1, NEG,     VAL_N_A, 0       },
	{ "MAP",         mupip_integ, 0, mup_integ_map_parm, 0, 0, 0, VAL_NOT_REQ,    1, NEG,     VAL_NUM, 0       },
	{ "MAXKEYSIZE",  mupip_integ, 0, mup_integ_map_parm, 0, 0, 0, VAL_NOT_REQ,    1, NEG,     VAL_NUM, 0       },
	{ "REGION",      mupip_integ, 0, 0,                  0, 0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0       },
	{ "SUBSCRIPT",   mupip_integ, 0, 0,                  0, 0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0       },
	{ "TN_RESET",    mupip_integ, 0, 0,                  0, 0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0       },
	{ "TRANSACTION", mupip_integ, 0, mup_integ_map_parm, 0, 0, 0, VAL_NOT_REQ,    1, NEG,     VAL_NUM, 0       },
	{ 0 }
};

static	CLI_PARM	mup_intrpt_parm[] = {
	{ "ID", "ID: " },
	{ "", "" }
};

static	CLI_PARM	mup_journal_parm[] = {
	{ "FILE", "Journal File(s): " },
	{ "", "" }
};

static readonly CLI_PARM mup_jnl_errlimit_parm[] = {
	{ "ERROR_LIMIT", 0 }
};

static readonly CLI_PARM mup_jnl_lost_fn[] = {
	{ "LOSTTRANS", "losttrans.mlt"}
};

static readonly CLI_PARM mup_jnl_lookback_parm[] = {
	{ "LOOKBACK_LIMIT", "TIME=0 00:05"}
};

static readonly CLI_PARM mup_jnl_fences_parm[] = {
	{ "FENCES", "PROCESS"}
};

static CLI_ENTRY mur_jnl_lookback_qual[] = {
	{ "OPERATIONS",	0, 0, 0, 0, 0, 0,            VAL_REQ,	0,	NON_NEG, VAL_NUM, 0 },
	{ "TIME",	0, 0, 0, 0, 0, DEFA_PRESENT, VAL_REQ,	0,	NON_NEG, VAL_STR, 0 },
	{ 0 }
};

static CLI_ENTRY mup_trans_qual[] = {
	{ "KILL",	0, 0, 0, 0, 0, 0, VAL_DISALLOWED, 0, NEG, VAL_N_A, 0 },
	{ "SET",	0, 0, 0, 0, 0, 0, VAL_DISALLOWED, 0, NEG, VAL_N_A, 0 },
	{ 0 }
};

static readonly CLI_PARM mup_jnl_show_parm[] = {
	{ "SHOW", "ALL"}
};

static	CLI_ENTRY	mup_journal_qual[] = {
{ "AFTER",             mupip_recover, 0, 0,                     0,                     0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
{ "APPLY_AFTER_IMAGE", mupip_recover, 0, 0,                     0,                     0, 0, VAL_DISALLOWED, 1, NEG,     VAL_N_A, 0 },
{ "BACKWARD",          mupip_recover, 0, 0,                     0,                     0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0 },
{ "BEFORE",            mupip_recover, 0, 0,                     0,                     0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
{ "BROKENTRANS",       mupip_recover, 0, 0,                     0,                     0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
{ "CHAIN",             mupip_recover, 0, 0,                     0,                     0, 0, VAL_DISALLOWED, 1, NEG,     VAL_N_A, 0 },
{ "CHECKTN",           mupip_recover, 0, 0,                     0,                     0, 0, VAL_DISALLOWED, 1, NEG,     VAL_N_A, 0 },
{ "DETAIL",            mupip_recover, 0, 0,                     0,                     0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0 },
{ "ERROR_LIMIT",       mupip_recover, 0, mup_jnl_errlimit_parm, 0,                     0, 0, VAL_NOT_REQ,    1, NEG,     VAL_NUM, 0 },
{ "EXTRACT",           mupip_recover, 0, 0,                     0,                     0, 0, VAL_NOT_REQ,    1, NON_NEG, VAL_STR, 0 },
{ "FENCES",            mupip_recover, 0, mup_jnl_fences_parm,   mup_jnl_fences_qual,   0, 0, VAL_NOT_REQ,    1, NON_NEG, VAL_STR, 0 },
{ "FETCHRESYNC",       mupip_recover, 0, 0,                     0,                     0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM, 0 },
{ "FORWARD",           mupip_recover, 0, 0,                     0,                     0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0 },
{ "FULL",              mupip_recover, 0, 0,                     0,                     0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0 },
{ "GLOBAL",            mupip_recover, 0, 0,                     0,                     0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
{ "ID",                mupip_recover, 0, 0,                     0,                     0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
{ "INTERACTIVE",       mupip_recover, 0, 0,                     0,                     0, 0, VAL_DISALLOWED, 1, NEG,     VAL_N_A, 0 },
{ "LOOKBACK_LIMIT",    mupip_recover, 0, mup_jnl_lookback_parm, mur_jnl_lookback_qual, 0, 0, VAL_NOT_REQ,    1, NEG,     VAL_STR, 0 },
{ "LOSTTRANS",         mupip_recover, 0, mup_jnl_lost_fn,       0,                     0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
{ "RECOVER",           mupip_recover, 0, 0,                     0,                     0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0 },
{ "REDIRECT",          mupip_recover, 0, 0,                     0,                     0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
{ "RESYNC",            mupip_recover, 0, 0,                     0,                     0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM, 0 },
{ "ROLLBACK",          mupip_recover, 0, 0,                     0,                     0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0 },
{ "SHOW",              mupip_recover, 0, mup_jnl_show_parm,     mup_jnl_show_qual,     0, 0, VAL_NOT_REQ,    1, NON_NEG, VAL_STR, 0 },
{ "SINCE",             mupip_recover, 0, 0,                     0,                     0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
{ "TRANSACTION",       mupip_recover, 0, 0,                     mup_trans_qual,        0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
{ "USER",              mupip_recover, 0, 0,                     0,                     0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
{ "VERBOSE",           mupip_recover, 0, 0,                     0,                     0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0 },
{ "VERIFY",            mupip_recover, 0, 0,                     0,                     0, 0, VAL_DISALLOWED, 1, NEG,     VAL_N_A, 0 },
{ 0 }
};

static	CLI_PARM	mup_load_parm[] = {
	{ "FILE", "Input File: " },
	{ "", "" }
};

static readonly CLI_PARM mup_load_ff_parm[] = {
	{ "FILL_FACTOR", "100"}
};

static readonly CLI_PARM mup_load_fmt_parm[] = {
	{ "FORMAT", "GO"}
};

static	CLI_ENTRY	mup_load_qual[] = {
	{ "BEGIN",         mupip_cvtgbl, 0, 0,                 0,                 0, 0, VAL_REQ,     1, NON_NEG, VAL_NUM, 0 },
	{ "BLOCK_DENSITY", mupip_cvtgbl, 0, 0,                 0,                 0, 0, VAL_NOT_REQ, 1, NON_NEG, VAL_NUM, 0 },
	{ "END",           mupip_cvtgbl, 0, 0,                 0,                 0, 0, VAL_REQ,     1, NON_NEG, VAL_NUM, 0 },
	{ "FILL_FACTOR",   mupip_cvtgbl, 0, mup_load_ff_parm,  0,                 0, 0, VAL_NOT_REQ, 1, NON_NEG, VAL_NUM, 0 },
	{ "FORMAT",        mupip_cvtgbl, 0, mup_load_fmt_parm, mup_load_fmt_qual, 0, 0, VAL_NOT_REQ, 1, NON_NEG, VAL_STR, 0 },
	{ "OCHSET",        mupip_cvtgbl, 0, 0,                 0,                 0, 0, VAL_REQ,     1, NON_NEG, VAL_STR, 0 },
	{ 0 }
};

static readonly CLI_PARM mup_reorg_ff_parm[] = {
	{ "FILL_FACTOR", "100"}
};

/* USER_DEFINED_REORG should not be documented to customers */
static  CLI_ENTRY       mup_reorg_qual[] = {
	{ "EXCLUDE",		mupip_reorg, 0, 0, 0, 0, 0, VAL_REQ,			1, NON_NEG, VAL_STR, 0 },
	{ "FILL_FACTOR",	mupip_reorg, 0, mup_reorg_ff_parm, 0, 0, 0, VAL_REQ,	1, NON_NEG, VAL_NUM, 0 },
	{ "INDEX_FILL_FACTOR",	mupip_reorg, 0, mup_reorg_ff_parm, 0, 0, 0, VAL_REQ,	1, NON_NEG, VAL_NUM, 0 },
	{ "RESUME",		mupip_reorg, 0, 0, 0, 0, 0, VAL_DISALLOWED,		1, NON_NEG, VAL_N_A, 0 },
	{ "SELECT",		mupip_reorg, 0, 0, 0, 0, 0, VAL_REQ,			1, NON_NEG, VAL_STR, 0 },
	{ "USER_DEFINED_REORG", mupip_reorg, 0, 0, 0, 0, 0, VAL_REQ,			1, NON_NEG, VAL_STR, 0 },
	{ 0 }
};

static readonly CLI_PARM gtmsource_timeout_parm[] = {
	{"TIMEOUT", "30"},
	{"", ""}
};

static CLI_ENTRY	gtmsource_qual[] = {
	{"ACTIVATE",         0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"BUFFSIZE",         0, 0, 0,                      0, 0, 0, VAL_REQ,        0, NON_NEG, VAL_NUM, 0 },
	{"CHANGELOG",        0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"CHECKHEALTH",      0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"CONNECTPARAMS",    0, 0, 0,                      0, 0, 0, VAL_REQ,        0, NON_NEG, VAL_STR, 0 },
	{"DEACTIVATE",       0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"FILTER",           0, 0, 0,                      0, 0, 0, VAL_REQ,        0, NON_NEG, VAL_STR, 0 },
	{"LOG",              0, 0, 0,                      0, 0, 0, VAL_REQ,        0, NON_NEG, VAL_STR, 0 },
	{"PASSIVE",          0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"SECONDARY",        0, 0, 0,                      0, 0, 0, VAL_REQ,        0, NON_NEG, VAL_STR, 0 },
	{"SHOWBACKLOG",      0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"SHUTDOWN",         0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"SOURCE",           0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"START",            0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"STATSLOG",         0, 0, 0,                      0, 0, 0, VAL_REQ,        0, NON_NEG, VAL_STR, 0 },
	{"STOPSOURCEFILTER", 0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"TIMEOUT",          0, 0, gtmsource_timeout_parm, 0, 0, 0, VAL_NOT_REQ,    1, NEG,     VAL_NUM, 0 },
	{"UPDATE",           0, 0, 0,                      0, 0, 0, VAL_REQ,        0, NON_NEG, VAL_STR, 0 },
	{ 0 }
};

static readonly CLI_PARM gtmrecv_timeout_parm[] = {
	{"TIMEOUT", "30"},
	{"", ""}
};

static CLI_ENTRY	gtmrecv_qual[] = {
	{"BUFFSIZE",         0, 0, 0,                      0, 0, 0, VAL_REQ,        0, NON_NEG, VAL_NUM, 0 },
	{"CHANGELOG",        0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"CHECKHEALTH",      0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"FILTER",           0, 0, 0,                      0, 0, 0, VAL_REQ,        0, NON_NEG, VAL_STR, 0 },
	{"LISTENPORT",       0, 0, 0,                      0, 0, 0, VAL_REQ,        0, NON_NEG, VAL_NUM, 0 },
	{"LOG",              0, 0, 0,                      0, 0, 0, VAL_REQ,        0, NON_NEG, VAL_STR, 0 },
	{"SHOWBACKLOG",      0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"SHUTDOWN",         0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"START",            0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"STATSLOG",         0, 0, 0,                      0, 0, 0, VAL_REQ,        0, NON_NEG, VAL_STR, 0 },
	{"STOPSOURCEFILTER", 0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"TIMEOUT",          0, 0, gtmsource_timeout_parm, 0, 0, 0, VAL_NOT_REQ,    1, NEG,     VAL_NUM, 0 },
	{"UPDATEONLY",       0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{"UPDATERESYNC",     0, 0, 0,                      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{ 0 }
};

static CLI_ENTRY	updproc_qual[] = {
	{ 0 }
};

static	CLI_ENTRY	inst_cre_qual[] = {
	{ 0 }
};

static	CLI_PARM	mup_replicate_parm[] = {
	{ "WHAT", "Source or Receiver: " },
	{ "", "" }
};

static	CLI_ENTRY	mup_replicate_qual[] = {
{ "INSTANCE_CREATE", repl_inst_create,         inst_cre_qual,  0, 0, 0,                                 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
{ "RECEIVER",        (void(*)(void))gtmrecv,   gtmrecv_qual,   0, 0, cli_disallow_mupip_replic_receive, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
{ "SOURCE",          (void(*)(void))gtmsource, gtmsource_qual, 0, 0, cli_disallow_mupip_replic_source,  0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
{ "UPDATEPROC",      (void(*)(void))updproc,   updproc_qual,   0, 0, 0,                                 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
{ 0 }
};

static	CLI_PARM	mup_restore_parm[] = {
	{ "DATABASE",   "Database: "      },
	{ "INPUT_FILE", "Input File(s): " },
	{ "", "" }
};

static	CLI_ENTRY	mup_restore_qual[] = {
	{ "EXTEND",     mupip_restore, 0, 0, 0, 0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0 },
	{ "NETTIMEOUT", mupip_restore, 0, 0, 0, 0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM, 0 },
	{ 0 }
};

static	CLI_PARM	mup_rundown_parm[] = {
	{ "WHAT", "File or Region: " },
	{ "", "" }
};

static	CLI_ENTRY	mup_rundown_qual[] = {
	{ "FILE",   mupip_rundown, 0, 0, 0, 0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0 },
	{ "REGION", mupip_rundown, 0, 0, 0, 0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A, 0 },
	{ 0 }
};

static	CLI_PARM	mup_set_parm[] = {
	{ "WHAT", "File or Region: " },
	{ "", "" }
};

static readonly CLI_PARM mup_set_ftime_parm[] = {
	{ "FLUSH_TIME", "100" }
};

static	CLI_ENTRY	mup_set_qual[] = {
{ "ACCESS_METHOD",        mupip_set, 0, 0,                  mup_set_acc_qual,     0, 0, VAL_REQ,        1, NON_NEG, VAL_STR,  0 },
{ "BYPASS",               mupip_set, 0, 0,                  0,                    0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A,  0 },
{ "DBFILENAME",           mupip_set, 0, 0,                  0,                    0, 0, VAL_REQ,        1, NEG,     VAL_STR,  0 },
{ "DEFER_TIME",           mupip_set, 0, 0,                  0,                    0, 0, VAL_REQ,        1, NEG,     VAL_STR,  0 },
{ "EXTENSION_COUNT",      mupip_set, 0, 0,                  0,                    0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM,  0 },
{ "FILE",                 mupip_set, 0, 0,                  0,                    0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A,  0 },
{ "FLUSH_TIME",           mupip_set, 0, mup_set_ftime_parm, 0,                    0, 0, VAL_NOT_REQ,    1, NON_NEG, VAL_TIME, 0 },
{ "GLOBAL_BUFFERS",       mupip_set, 0, 0,                  0,                    0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM,  0 },
{ "JNLFILE",              mupip_set, 0, 0,                  0,                    0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A,  0 },
{ "JOURNAL",              mupip_set, 0, 0,                  mup_set_journal_qual, 0, 0, VAL_NOT_REQ,    1, NEG,     VAL_STR,  0 },
{ "LOCK_SPACE",           mupip_set, 0, 0,                  0,                    0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM,  0 },
{ "PARTIAL_RECOV_BYPASS", mupip_set, 0, 0,                  0,                    0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A,  0 },
{ "PREVJNLFILE",          mupip_set, 0, 0,                  0,                    0, 0, VAL_REQ,        1, NEG,     VAL_STR,  0 },
{ "REGION",               mupip_set, 0, 0,                  0,                    0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A,  0 },
{ "REPLICATION",          mupip_set, 0, 0,                  mup_set_repl_qual,    0, 0, VAL_REQ,        1, NEG,     VAL_STR,  0 },
{ "REPL_STATE",           mupip_set, 0, 0,                  0,                    0, 0, VAL_REQ,        1, NEG,     VAL_STR,  0 },
{ "RESERVED_BYTES",       mupip_set, 0, 0,                  0,                    0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM,  0 },
{ "STANDALONENOT",        mupip_set, 0, 0,                  0,                    0, 0, VAL_DISALLOWED, 1, NON_NEG, VAL_N_A,  0 },
{ "WAIT_DISK",            mupip_set, 0, 0,                  0,                    0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM,  0 },
{ 0 }
};

static	CLI_PARM	mup_stop_parm[] = {
	{ "ID", "ID: " },
	{ "", "" }
};

static	CLI_PARM	mup_upgrade_parm[] = {
	{ "FILE", "File: " },
	{ "", "" }
};

GBLDEF	CLI_ENTRY	cmd_ary[] = {
{ "BACKUP",    mupip_backup,  mup_backup_qual,    mup_backup_parm,    0, cli_disallow_mupip_backup,    0, VAL_DISALLOWED, 2, 0, 0, 0 },
{ "CONVERT",   mupip_cvtpgm,  mup_convert_qual,   mup_convert_parm,   0, 0,                            0, VAL_DISALLOWED, 2, 0, 0, 0 },
{ "CREATE",    mupip_create,  mup_create_qual,    0,                  0, 0,                            0, VAL_DISALLOWED, 0, 0, 0, 0 },
{ "EXIT",      mupip_quit,    0,                  0,                  0, 0,                            0, VAL_DISALLOWED, 0, 0, 0, 0 },
{ "EXTEND",    mupip_extend,  mup_extend_qual,    mup_extend_parm,    0, 0,                            0, VAL_DISALLOWED, 1, 0, 0, 0 },
{ "EXTRACT",   mu_extract,    mup_extract_qual,   mup_extract_parm,   0, 0,                            0, VAL_DISALLOWED, 1, 0, 0, 0 },
{ "FREEZE",    mupip_freeze,  mup_freeze_qual,    mup_freeze_parm,    0, cli_disallow_mupip_freeze,    0, VAL_DISALLOWED, 1, 0, 0, 0 },
{ "FTOK",      mupip_ftok,    mup_ftok_qual,      mup_ftok_parm,      0, 0,                            0, VAL_DISALLOWED, 1, 0, 0, 0 },
{ "HELP",      mupip_help,    0,                  0,                  0, 0,                            0, VAL_DISALLOWED, 0, 0, 0, 0 },
{ "INTEG",     mupip_integ,   mup_integ_qual,     mup_integ_parm,     0, cli_disallow_mupip_integ,     0, VAL_DISALLOWED, 1, 0, 0, 0 },
{ "INTRPT",    mupip_intrpt,  0,                  mup_intrpt_parm,    0, 0,                            0, VAL_DISALLOWED, 1, 0, 0, 0 },
{ "JOURNAL",   mupip_recover, mup_journal_qual,   mup_journal_parm,   0, cli_disallow_mupip_journal,   0, VAL_DISALLOWED, 1, 0, 0, 0 },
{ "LOAD",      mupip_cvtgbl,  mup_load_qual,      mup_load_parm,      0, 0,                            0, VAL_DISALLOWED, 1, 0, 0, 0 },
{ "QUIT",      mupip_quit,    0,                  0,                  0, 0,                            0, VAL_DISALLOWED, 0, 0, 0, 0 },
{ "REORG",     mupip_reorg,   mup_reorg_qual,     0,                  0, 0,                            0, VAL_DISALLOWED, 2, 0, 0, 0 },
{ "REPLICATE", 0,             mup_replicate_qual, mup_replicate_parm, 0, cli_disallow_mupip_replicate, 0, VAL_DISALLOWED, 1, 0, 0, 0 },
{ "RESTORE",   mupip_restore, mup_restore_qual,   mup_restore_parm,   0, 0,                            0, VAL_DISALLOWED, 2, 0, 0, 0 },
{ "RUNDOWN",   mupip_rundown, mup_rundown_qual,   mup_rundown_parm,   0, cli_disallow_mupip_rundown,   0, VAL_DISALLOWED, 1, 0, 0, 0 },
{ "SET",       mupip_set,     mup_set_qual,       mup_set_parm,       0, cli_disallow_mupip_set,       0, VAL_DISALLOWED, 1, 0, 0, 0 },
{ "STOP",      mupip_stop,    0,                  mup_stop_parm,      0, 0,                            0, VAL_DISALLOWED, 1, 0, 0, 0 },
{ "UPGRADE",   mupip_upgrade, 0,                  mup_upgrade_parm,   0, 0,                            0, VAL_DISALLOWED, 1, 0, 0, 0 },
{ 0 }
};

