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

#include <limits.h>
#include <netinet/in.h>
#include <stddef.h>
#include <errno.h>

#include "gtm_stdio.h"
#include "gtm_inet.h"
#include "gtm_string.h"
#include "gtm_stdlib.h"
#include "gtm_time.h"
#include "gtm_stat.h"

#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "mlkdef.h"
#include "gdscc.h"
#include "cmidef.h"
#include "hashdef.h"
#include "cmmdef.h"
#include "cmi.h"
#include "gt_timer.h"
#include "gtcmlkdef.h"
#include "stp_parms.h"
#include "stringpool.h"
#include "repl_msg.h"
#include "gtmsource.h"
#include "gtmimagename.h"
#include "gtcmtr_kill.h"
#include "gtcmtr_put.h"
#include "trans_log_name.h"
#include "get_page_size.h"
#include "filestruct.h"
#include "jnl.h"
#include "gdskill.h"
#include "hashtab.h"
#include "buddy_list.h"
#include "tp.h"
#include "init_secshr_addrs.h"
#include "cli.h"
#include "cli_parse.h"
#include "iosp.h"
#include "error.h"
#include "gtcml.h"
#include "gtcmtr_lk.h"
#include "gtcmtr_lke_clear.h"
#include "gtcmtr_lke_show.h"
#include "getjobnum.h"
#include "cache.h"
#include "gtmmsg.h"
#include "dpgbldir.h"
#include "sig_init.h"
#include "patcode.h"
#include "copy.h"
#include "lockconst.h"
#include "generic_signal_handler.h"
#include "gtcmtr_data.h"
#include "gtcmtr_get.h"
#include "gtcmtr_order.h"
#include "gtcmtr_zprevious.h"
#include "gtcmtr_query.h"
#include "gtcmtr_zwithdraw.h"
#include "gtcmtr_initproc.h"
#include "gtcmtr_initreg.h"
#include "gtcmtr_terminate.h"
#include "gtcmtr_bufflush.h"
#include "gtcm_shutdown_ast.h"
#include "gtcm_neterr.h"
#include "gtcm_link_accept.h"
#include "gtcm_remove_from_action_queue.h"
#include "gtcm_read_ast.h"
#include "gtcm_write_ast.h"
#include "gtcm_int_unpack.h"
#include "gtcm_init_ast.h"
#include "gtcm_urgread_ast.h"
#include "gtcm_exi_handler.h"
#include "mu_gv_cur_reg_init.h"
#include "gtcm_open_cmerrlog.h"
#include "gtcm_gnp_pktdmp.h"
#include "util.h"
#include "getzdir.h"
#include "gtm_env_init.h"	/* for gtm_env_init() prototype */

#ifdef __osf__
#pragma pointer_size (save)
#pragma pointer_size (long)
#endif

GBLDEF char **gtmenvp;
GBLREF char cli_err_str[];

#ifdef __osf__
#pragma pointer_size (restore)
#endif

GBLDEF bool			cm_timeout = FALSE;
GBLDEF bool			cm_shutdown = FALSE;
GBLDEF unsigned short		procnum;
GBLDEF int			gtcm_users = 0;
GBLDEF int4			gtcm_exi_condition;
GBLDEF connection_struct 	*curr_entry;
GBLDEF relque ALIGN_QUAD	action_que;
GBLDEF struct CLB       	*proc_to_clb[USHRT_MAX + 1];    /* for index 0 */
GBLDEF gd_region		*action_que_dummy_reg;
/* the file is the actual file being used */
GBLDEF char			gtcm_gnp_server_log[MAX_FN_LEN + 1];
/* the length is the orignal length */
GBLDEF int			gtcm_gnp_log_path_len;
GBLREF FILE			*gtcm_errfs;
GBLREF bool			certify_all_blocks;
GBLREF bool			licensed;
GBLREF bool			run_time;
GBLREF boolean_t		gtcm_connection;
GBLREF unsigned char		cw_set_depth;
GBLREF uint4			process_id;
GBLREF cm_lckblkreg		*blkdlist;
GBLREF cw_set_element		cw_set[];
GBLREF gd_region		*gv_cur_region;
GBLREF sgmnt_addrs		*cs_addrs;
GBLREF sgmnt_data_ptr_t		cs_data;
GBLREF gv_namehead		*gv_target;
GBLREF struct NTD		*ntd_root;
GBLREF jnlpool_addrs		jnlpool;
GBLREF spdesc			rts_stringpool, stringpool;
GBLREF enum gtmImageTypes	image_type;
GBLREF IN_PARMS			*cli_lex_in_ptr;
GBLREF char			cli_token_buf[];
GBLREF boolean_t		is_replicator;

OS_PAGE_SIZE_DECLARE

static uint4			closewait;

#define CM_SERV_WAIT_FOR_INPUT	100 /* ms */
#define CM_CLB_POOL_SIZE		32

static void gtcm_gnp_server_actions(void);
static void gtcm_gnp_switch_interrupt(int sig);
static void gtcm_gnp_trace(struct CLB *lnk, int sta, unsigned char *buf, size_t len);
static void gtcm_gnp_trace_on(int sig);
static void gtcm_gnp_trace_off(int sig);

static VSIG_ATOMIC_T switch_log = FALSE;
static VSIG_ATOMIC_T trace_on = FALSE;

static void gtcm_gnp_server_actions(void)
{
	int4			status;
	unsigned short		value;
	char			reply;
	connection_struct	*prev_curr_entry;
	CMI_MUTEX_DECL;

	error_def(CMERR_CMINTQUE);
	error_def(ERR_BADGTMNETMSG);

	ESTABLISH(gtcm_ch);
	while (!cm_shutdown)
	{
		if (switch_log)
		{
			gtcm_open_cmerrlog();
			switch_log = FALSE;
		}
		if (trace_on)
		{
			if (!ntd_root->trc)
				ntd_root->trc = gtcm_gnp_trace;
		}
		else
		{
			if (ntd_root->trc)
				ntd_root->trc = NULL;
		}
		if (blkdlist)
			gtcml_chkreg();
		CMI_MUTEX_BLOCK;
		gtcm_remove_from_action_queue();
		CMI_MUTEX_RESTORE;
		if ((connection_struct *)INTERLOCK_FAIL == curr_entry)
			rts_error(VARLSTCNT(1) CMERR_CMINTQUE);
		if ((connection_struct *)EMPTY_QUEUE != curr_entry)
		{
			if (1 == (curr_entry->int_cancel.laflag & 1))
			{ /* valid interrupt cancel msg, handle in gtcm_urgread_ast */
				CMI_MUTEX_BLOCK;
				prev_curr_entry = curr_entry;
				curr_entry = EMPTY_QUEUE;
				gtcm_int_unpack(prev_curr_entry);
				CMI_MUTEX_RESTORE;
				continue;
			}
			switch (*curr_entry->clb_ptr->mbf)
			{
				case CMMS_L_LKCANALL:
					reply = gtcmtr_lkcanall();
					break;
				case CMMS_L_LKCANCEL:
					reply = gtcmtr_lkcancel();
					break;
				case CMMS_L_LKREQIMMED:
					reply = gtcmtr_lkreqimmed();
					break;
				case CMMS_L_LKREQNODE:
					reply = gtcmtr_lkreqnode();
					break;
				case CMMS_L_LKREQUEST:
					reply = gtcmtr_lkrequest();
					break;
				case CMMS_L_LKRESUME:
					reply = gtcmtr_lkresume();
					break;
				case CMMS_L_LKACQUIRE:
					reply = gtcmtr_lkacquire();
					break;
				case CMMS_L_LKSUSPEND:
					reply = gtcmtr_lksuspend();
					break;
				case CMMS_L_LKDELETE:
					reply = gtcmtr_lkdelete();
					break;
				case CMMS_Q_DATA:
					reply = gtcmtr_data();
					break;
				case CMMS_Q_GET:
					reply = gtcmtr_get();
					break;
				case CMMS_Q_KILL:
					reply = gtcmtr_kill();
					break;
				case CMMS_Q_ORDER:
					reply = gtcmtr_order();
					break;
				case CMMS_Q_PREV:
					reply = gtcmtr_zprevious();
					break;
				case CMMS_Q_PUT:
					reply = gtcmtr_put();
					break;
				case CMMS_Q_QUERY:
					reply = gtcmtr_query();
					break;
				case CMMS_Q_ZWITHDRAW:
					reply = gtcmtr_zwithdraw();
					break;
				case CMMS_S_INITPROC:
					reply = gtcmtr_initproc();
					break;
				case CMMS_S_INITREG:
					reply = gtcmtr_initreg();
					break;
				case CMMS_S_TERMINATE:
					reply = gtcmtr_terminate(TRUE);
					break;
				case CMMS_E_TERMINATE:
					reply = gtcmtr_terminate(FALSE);
					break;
#ifdef notdef
				case CMMS_U_LKEDELETE:
					reply = gtcmtr_lke_clearrep(curr_entry->clb_ptr, (clear_request *)curr_entry->clb_ptr->mbf);
					break;
				case CMMS_U_LKESHOW:
					reply = gtcmtr_lke_showrep(curr_entry->clb_ptr, (show_request *)curr_entry->clb_ptr->mbf);
					break;
#endif
				case CMMS_B_BUFRESIZE:
					reply = CM_WRITE;
					GET_USHORT(value, curr_entry->clb_ptr->mbf + 1);
					value += CM_BUFFER_OVERHEAD;
					if (value > curr_entry->clb_ptr->mbl)
						cmi_realloc_mbf(curr_entry->clb_ptr, value);
					*curr_entry->clb_ptr->mbf = CMMS_C_BUFRESIZE;
					curr_entry->clb_ptr->cbl = 1;
					break;
				case CMMS_B_BUFFLUSH:
					reply = gtcmtr_bufflush();
					break;
				default:
					reply = FALSE;
					if (SS_NORMAL == status)
					{
						GET_LONG(status, curr_entry->clb_ptr->mbf);
                                                rts_error(VARLSTCNT(3) ERR_BADGTMNETMSG, 1, status);
					}
					break;
			}
			if (curr_entry)		/* curr_entry can be NULL if went through gtcmtr_terminate */
			{
				time((time_t *)&curr_entry->lastact[0]);
				/* curr_entry is used by gtcm_urgread_ast to determine if it needs to defer the interrupt message */
				prev_curr_entry = curr_entry;
				if (CM_WRITE == reply)
				{	/* if ast == gtcm_write_ast, let it worry */
					curr_entry->clb_ptr->ast = gtcm_write_ast;
					curr_entry = EMPTY_QUEUE;
					cmi_write(prev_curr_entry->clb_ptr);
				} else
				{
					curr_entry = EMPTY_QUEUE;
					if (1 == (prev_curr_entry->int_cancel.laflag & 1))
					{  /* valid interrupt cancel msg, handle in gtcm_urgread_ast */
						CMI_MUTEX_BLOCK;
						gtcm_int_unpack(prev_curr_entry);
						CMI_MUTEX_RESTORE;
					} else if (CM_READ == reply)
					{
						prev_curr_entry->clb_ptr->ast = gtcm_read_ast;
						cmi_read(prev_curr_entry->clb_ptr);
					}
				}
			}
		} else
			CMI_IDLE(CM_SERV_WAIT_FOR_INPUT);
		if (cm_timeout && (0 == gtcm_users))
                        start_timer((TID)&cm_shutdown, closewait, gtcm_shutdown_ast, 0, NULL);
	}
	return;
}

static void gtcm_gnp_trace(struct CLB *lnk, int sta, unsigned char *buf, size_t len)
{
	gtcm_gnp_cpktdmp(gtcm_errfs, lnk, sta, buf, len, "");
}

static void gtcm_gnp_trace_on(int sig)
{
	trace_on = TRUE;
}

static void gtcm_gnp_trace_off(int sig)
{
	trace_on = FALSE;
}

static void gtcm_gnp_switch_interrupt(int sig)
{
	switch_log = TRUE;
}


#ifdef __osf__
#pragma pointer_size (save)
#pragma pointer_size (long)
#endif

int main(int argc, char **argv, char **envp)

#ifdef __osf__
#pragma pointer_size (restore)
#endif
{
	int4			timout;
	cmi_status_t		status;
	int			eof, arg_index, parse_ret;
	mstr			name1, name2;
	mstr			node_name;
	cmi_descriptor		service_descr, log_path_descr;
	unsigned short		service_len, log_path_len;
	char			nbuff[256], *ptr, service[512];
	now_t			now;	/* for GET_CUR_TIME macro */
	char			time_str[CTIME_BEFORE_NL + 2], *time_ptr; /* for GET_CUR_TIME macro */
	pid_t			pid;
	struct sigaction	act;

        static boolean_t no_fork = FALSE;

	error_def(ERR_NETFAIL);
	error_def(ERR_TEXT);

	image_type = GTCM_GNP_SERVER_IMAGE;
	is_replicator = TRUE;	/* as GT.CM GNP goes through t_end() and can write jnl records to the jnlpool for replicated db */
	gtm_env_init();	/* read in all environment variables */
	gtmenvp = envp;
	run_time = TRUE;
	getjobnum();
	err_init(stop_image_conditional_core);
	assert(0 == offsetof(gv_key, top)); /* for integrity of CM_GET_GVCURRKEY */
	assert(2 == offsetof(gv_key, end)); /* for integrity of CM_GET_GVCURRKEY */
	assert(4 == offsetof(gv_key, prev)); /* for integrity of CM_GET_GVCURRKEY */
	/* read comments in gtm.c for cli magic below */
	cli_lex_setup(argc, argv);
	if (1 < argc)
		cli_gettoken(&eof);
	cli_token_buf[0] = '\0';
	ptr = cli_lex_in_ptr->in_str;
	memmove(ptr + sizeof("GTCM_GNP_SERVER ") - 1, ptr, strlen(ptr));
	memcpy(ptr, "GTCM_GNP_SERVER ", sizeof("GTCM_GNP_SERVER ") - 1);
	cli_lex_in_ptr->tp = cli_lex_in_ptr->in_str;
	parse_ret = parse_cmd();
	if (parse_ret && (EOF != parse_ret))
		rts_error(VARLSTCNT(4) parse_ret, 2, LEN_AND_STR(cli_err_str));
	service_len = (unsigned short)sizeof(service);
	CMI_DESC_SET_POINTER(&service_descr, service);
	service[0] = '\0';
	if (CLI_PRESENT == cli_present("SERVICE") && cli_get_str("SERVICE", CMI_DESC_POINTER(&service_descr), &service_len))
		CMI_DESC_SET_LENGTH(&service_descr, service_len);
	else
		CMI_DESC_SET_LENGTH(&service_descr, 0);
	if (cli_get_num("TIMEOUT", &timout))
	{
		cm_timeout = TRUE;
		if (timout > (1 << 21))
			timout = (1 << 21);
		closewait = (timout << 10); /* s -> ms; approx */
	}
	log_path_len = (unsigned short)sizeof(gtcm_gnp_server_log) - 1;
	CMI_DESC_SET_POINTER(&log_path_descr, gtcm_gnp_server_log);
	if (CLI_PRESENT != cli_present("LOG") || !cli_get_str("LOG", CMI_DESC_POINTER(&log_path_descr), &log_path_len))
		log_path_len = 0;
	if (CLI_PRESENT == cli_present("TRACE"))
		trace_on = TRUE;
	gtcm_gnp_server_log[log_path_len] = '\0';
	gtcm_open_cmerrlog();
        assert(0 == EMPTY_QUEUE);
	get_page_size();
	licensed = TRUE;
	stp_init(STP_INITSIZE);
	rts_stringpool = stringpool;
	cache_init();
	getzdir();
	sig_init(generic_signal_handler, null_handler); /* should do be done before cmi_init */

	/* Redefine handler for SIGHUP to switch log file */
	memset(&act, 0, sizeof(act));
	act.sa_handler  = gtcm_gnp_switch_interrupt;
	sigaction(SIGHUP, &act, 0);
	act.sa_handler  = gtcm_gnp_trace_on;
	sigaction(SIGUSR1, &act, 0);
	act.sa_handler  = gtcm_gnp_trace_off;
	sigaction(SIGUSR2, &act, 0);

	procnum = 0;
        memset(proc_to_clb, 0, sizeof(proc_to_clb));
	node_name.addr = nbuff;
	node_name.len = 0;

	/* child continues here */
	gtcm_connection = FALSE;
        if (!no_fork)
        {
                if ((pid = fork()) < 0)
                {
			rts_error(VARLSTCNT(5) ERR_TEXT, 2, LEN_AND_LIT("Error forking gnp server into the background"), errno);
                        exit(-1);
                }
                else if (pid > 0)
                        exit(0);
		getjobnum();
                (void) setpgrp();
        }
	/* Write argv and the process id for ease of admin */
	GET_CUR_TIME;
	util_out_print("!AD : ", FALSE, CTIME_BEFORE_NL, time_ptr);
	for (arg_index = 0; arg_index < argc; arg_index++)
		util_out_print("!AZ ", FALSE, argv[arg_index]);
	util_out_print("[pid : !UL]", TRUE, process_id);
	/*
       	 * the acc function pointer is NULL to prevent incoming connections
	 * until we are ready.
	 */
	status = cmi_init(&service_descr, '\0', gtcm_neterr, NULL, gtcm_link_accept, gtcm_urgread_ast, CM_CLB_POOL_SIZE,
			  sizeof(connection_struct), CM_MSG_BUF_SIZE + CM_BUFFER_OVERHEAD);
	if (CMI_ERROR(status))
	{
		gtm_putmsg(VARLSTCNT(7) ERR_NETFAIL, 0, ERR_TEXT, 2, LEN_AND_LIT("Network interface initialization failed"),
				status);
		exit(status);
	}
	atexit(gtcm_exi_handler);
	init_secshr_addrs(get_next_gdr, cw_set, NULL, &cw_set_depth, process_id, OS_PAGE_SIZE, &jnlpool.jnlpool_dummy_reg);
	initialize_pattern_table();

	/* Pre-allocate some timer blocks. */
	prealloc_gt_timers();

	name1.addr = "GTCM_GDSCERT";
	name1.len = sizeof("GTCM_GDSCERT") - 1;
	if (SS_NORMAL == (status = trans_log_name(&name1, &name2, nbuff)))
		certify_all_blocks = TRUE;
	SET_LATCH_GLOBAL(&action_que.latch, LOCK_AVAILABLE);
	mu_gv_cur_reg_init();
	cs_addrs = &FILE_INFO(gv_cur_region)->s_addrs;
	cs_data = cs_addrs->hdr;
	cs_addrs->nl = (node_local_ptr_t)malloc(sizeof(node_local));
	action_que_dummy_reg = gv_cur_region;
	/* ... now we are ready! */
	ntd_root->crq = gtcm_init_ast;
	while (!cm_shutdown)
	{
		gtcm_gnp_server_actions();
	}
	exit(SS_NORMAL);
}
