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

 /* General repository for global variable definitions. This keeps us from
   pulling in modules and all their references when all we wanted was the
   global data def.. */

#include "mdef.h"

#include "gtm_inet.h"
#include "gtm_iconv.h"
#include "gtm_socket.h"
#include "gtm_unistd.h"

#include <limits.h>
#include <signal.h>
#include <netinet/in.h>		/* Required for gtmsource.h */
#ifdef __MVS__
#include <time.h>      /* required for fd_set */
#include <sys/time.h>
#endif
#ifdef UNIX
# include <sys/un.h>
#endif
#ifdef VMS
# include <descrip.h>		/* Required for gtmsource.h */
# include <ssdef.h>
# include "desblk.h"
#endif
#include "gdsroot.h"
#include "gdskill.h"
#include "ccp.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "gdscc.h"
#include "cache.h"
#include "comline.h"
#include "compiler.h"
#include "hashdef.h"
#include "hashtab.h"		/* needed also for tp.h */
#include "cmd_qlf.h"
#include "io.h"
#include "iosp.h"
#include "jnl.h"
#include "lv_val.h"
#include "sbs_blk.h"
#include "mdq.h"
#include "mprof.h"
#include "mv_stent.h"
#include "rtnhdr.h"
#include "stack_frame.h"
#include "stp_parms.h"
#include "stringpool.h"
#include "buddy_list.h"		/* needed for tp.h */
#include "tp.h"
#include "tp_frame.h"
#include "mlkdef.h"
#include "zshow.h"
#include "zwrite.h"
#include "zbreak.h"
#include "fnpc.h"
#include "mmseg.h"
#ifndef VMS
# include "gtmsiginfo.h"
#endif
#include "gtmimagename.h"
#include "iotcpdef.h"
#include "gt_timer.h"
#include "iosocketdef.h"	/* needed for socket_pool */
#include "ctrlc_handler_dummy.h"
#include "unw_prof_frame_dummy.h"
#include "op.h"
#include "gtmsecshr.h"
#include "error_trap.h"
#include "patcode.h"	/* for pat_everything and sizeof_pat_everything */

/* FOR REPLICATION RELATED GLOBALS */
#include "repl_msg.h"
#include "gtmsource.h"
#include "gtmrecv.h"

/* FOR MERGE RELATED GLOBALS */
#include "subscript.h"
#include "lvname_info.h"
#include "gvname_info.h"
#include "op_merge.h"

#ifdef UNIX
#include "cli.h"
#include "invocation_mode.h"
#include "fgncal.h"
#endif
#include "jnl_typedef.h"

#ifdef VMS
#include "gtm_logicals.h"	/* for GTM_MEMORY_NOACCESS_COUNT */
#endif

#define DEFAULT_ZERROR_STR	"Unprocessed $ZERROR, see $ZSTATUS"
#define DEFAULT_ZERROR_LEN	(sizeof(DEFAULT_ZERROR_STR) - 1)

GBLDEF	gd_region		*db_init_region;
GBLDEF	sgmnt_data_ptr_t	cs_data;
GBLDEF	sgmnt_addrs		*cs_addrs;

GBLDEF	unsigned short	proc_act_type;
GBLDEF	volatile bool	ctrlc_pending;
GBLDEF	volatile int4	ctrap_action_is;
GBLDEF	bool		out_of_time;
GBLDEF	io_pair		io_curr_device;		/* current device	*/
GBLDEF	io_pair		io_std_device;		/* standard device	*/
GBLDEF	io_log_name	*dollar_principal;	/* pointer to log name GTM$PRINCIPAL if defined */
GBLDEF	bool		prin_in_dev_failure = FALSE;
GBLDEF	bool		prin_out_dev_failure = FALSE;
GBLDEF	io_desc		*active_device;

GBLDEF	bool		error_mupip = FALSE,
			compile_time = FALSE,
			file_backed_up = FALSE,
			gv_replopen_error = FALSE,
			gv_replication_error = FALSE,
			incremental = FALSE,
			jobpid = FALSE,
			online = FALSE,
			record = FALSE,
			run_time = FALSE,
			is_standalone = FALSE,
			std_dev_outbnd = FALSE,
			in_mupip_freeze = FALSE,
	                in_backup = FALSE,
	                view_debug1 = FALSE,
	                view_debug2 = FALSE,
	                view_debug3 = FALSE,
	                view_debug4 = FALSE;

GBLDEF	boolean_t	is_updproc = FALSE,
			mupip_jnl_recover = FALSE,
			set_resync_to_region = FALSE,
			repl_enabled = FALSE,
			unhandled_stale_timer_pop = FALSE,
			gtcm_connection = FALSE,
			is_replicator = FALSE,	/* TRUE => this process can write jnl records to the jnlpool for replicated db */
	                tp_in_use = FALSE,	/* TRUE => TP has been used by this process and is thus initialized */
			dollar_truth = TRUE;

GBLDEF	VSIG_ATOMIC_T	forced_exit = FALSE;	/* Asynchronous signal/interrupt handler sets this variable to TRUE,
						 * hence the VSIG_ATOMIC_T type in the definition.
						 */
GBLDEF	unsigned char	*msp,
			*mubbuf,
			*restart_ctxt,
			*stackbase,
			*stacktop,
			*stackwarn;
GBLDEF	int4		backup_close_errno,
			backup_write_errno,
			mubmaxblk,
			forced_exit_err,
			exit_state,
			restore_read_errno;
GBLDEF	volatile int4	outofband, crit_count = 0;
GBLDEF	int		mumps_status = SS_NORMAL,
			restart_pc,
			stp_array_size = 0;
GBLDEF	cache_entry	*cache_entry_base, *cache_entry_top, *cache_hashent, *cache_stealp, cache_temps;
GBLDEF	cache_tabent	*cache_tabent_base;
GBLDEF	int		cache_hits, cache_fails, cache_temp_cnt;
GBLDEF	gvzwrite_struct	gvzwrite_block;
GBLDEF	io_log_name	*io_root_log_name;
GBLDEF	hashtab		*stp_duptbl = NULL;
GBLDEF	lvzwrite_struct	lvzwrite_block;
GBLDEF	mliteral	literal_chain;
GBLDEF	mstr		*comline_base,
			dollar_zsource,
			*err_act,
			**stp_array,
			extnam_str = {0, NULL},
			env_gtm_env_xlate = {0, NULL};
GBLDEF MSTR_CONST(default_sysid, "gtm_sysid");
GBLDEF int              (*gtm_env_xlate_entry)() = NULL;
GBLDEF void             (*gtm_sigusr1_handler)() = NULL;
GBLDEF	mval		dollar_zgbldir,
			dollar_zstatus,
			dollar_zstep = DEFINE_MVAL_LITERAL(MV_STR | MV_NM | MV_INT | MV_NUM_APPROX, 0, 0, 1, "B", 0, 0),
			dollar_ztrap,
			ztrap_pop2level = DEFINE_MVAL_LITERAL(MV_INT, 0, 0, 0, 0, 0, 0),
			zstep_action,
			dollar_system,
			dollar_estack_delta = DEFINE_MVAL_LITERAL(MV_STR, 0, 0, 0, NULL, 0, 0),
			dollar_etrap,
			dollar_zerror = DEFINE_MVAL_LITERAL(MV_STR, 0, 0, DEFAULT_ZERROR_LEN, DEFAULT_ZERROR_STR, 0, 0),
			dollar_zyerror,
			dollar_ztexit = DEFINE_MVAL_LITERAL(MV_STR, 0, 0, 0, NULL, 0, 0);
GBLDEF  uint4		dollar_zjob;
GBLDEF	mval		dollar_zinterrupt;
GBLDEF	boolean_t	dollar_zininterrupt;
GBLDEF	boolean_t	dollar_ztexit_bool; /* Truth value of dollar_ztexit when coerced to boolean */

GBLDEF	mv_stent	*mv_chain;
GBLDEF	sgm_info	*first_sgm_info;
GBLDEF	spdesc		indr_stringpool,
			rts_stringpool,
			stringpool;
GBLDEF	stack_frame	*frame_pointer;
GBLDEF	stack_frame	*zyerr_frame = NULL;
GBLDEF	symval		*curr_symval;
GBLDEF	tp_frame	*tp_pointer;
GBLDEF	tp_region	*halt_ptr,
			*grlist;
GBLDEF	trans_num	local_tn;	/* transaction number for THIS PROCESS (starts at 0 each time) */
GBLDEF	gv_namehead	*gv_target;
GBLDEF	gv_namehead	*gv_target_list;
GBLDEF	int4		exi_condition;
GBLDEF	uint4		gtmDebugLevel;
GBLDEF	caddr_t		smCallerId;			/* Caller of top level malloc/free */
GBLDEF	int		process_exiting;
GBLDEF	int4		dollar_zsystem;
GBLDEF	int4		dollar_zeditor;
GBLDEF	boolean_t	sem_incremented = FALSE;
GBLDEF	boolean_t	new_dbinit_ipc = FALSE;
GBLDEF	mval		**ind_result_array, **ind_result_sp, **ind_result_top;
GBLDEF	mval		**ind_source_array, **ind_source_sp, **ind_source_top;
GBLDEF	RTN_TABENT	*rtn_fst_table, *rtn_names, *rtn_names_top, *rtn_names_end;
GBLDEF	int4		break_message_mask;
GBLDEF	bool		rc_locked = FALSE,
			certify_all_blocks = FALSE;	/* If flag is set all blocks are checked after they are
							 * written to the database.  Upon error we stay critical
							 * and report.  This flag can be set via the MUMPS command
							 * VIEW 1. */
GBLDEF	mval		curr_gbl_root;
GBLDEF	gd_addr		*original_header;
GBLDEF	mem_list	*mem_list_head;
GBLDEF	boolean_t	debug_mupip;
GBLDEF	unsigned char	t_fail_hist[CDB_MAX_TRIES];
GBLDEF	cache_rec_ptr_t	cr_array[((MAX_BT_DEPTH * 2) - 1) * 2];	/* Maximum number of blocks that can be in transaction */
GBLDEF	unsigned int	cr_array_index;
GBLDEF	boolean_t	need_core;		/* Core file should be created */
GBLDEF	boolean_t	created_core;		/* core file was created */
GBLDEF	boolean_t	core_in_progress;	/* creating core NOW */
GBLDEF	boolean_t	dont_want_core;		/* Higher level flag overrides need_core set by lower level rtns */
GBLDEF	boolean_t	exit_handler_active;	/* recursion prevention */
GBLDEF	boolean_t	block_saved;
GBLDEF	iconv_t		dse_over_cvtcd = (iconv_t)0;
GBLDEF	short int	last_source_column;
GBLDEF	char		window_token;
GBLDEF	mval		window_mval;
GBLDEF	mident		window_ident;
GBLDEF	char		director_token;
GBLDEF	mval		director_mval;
GBLDEF	mident		director_ident;
GBLDEF	char		*lexical_ptr;
GBLDEF	int4		aligned_source_buffer[MAX_SRCLINE / sizeof(int4) + 1];
GBLDEF	unsigned char	*source_buffer = (unsigned char *)aligned_source_buffer;
GBLDEF	int4		source_error_found;
GBLDEF	src_line_struct	src_head;
GBLDEF	bool		code_generated;
GBLDEF	short int	source_column, source_line;
GBLDEF	bool		devctlexp;
GBLDEF 	char		cg_phase;       /* code generation phase */
/* Previous code generation phase: Only used by emit_code.c to initialize the push list at the
 * beginning of each phase (bug fix: C9D12-002478) */
GBLDEF 	char		cg_phase_last;

GBLDEF	int		cmd_cnt;

GBLDEF	command_qualifier	glb_cmd_qlf = { CQ_DEFAULT },
				cmd_qlf = { CQ_DEFAULT };
#ifdef __osf__
#pragma pointer_size (save)
#pragma pointer_size (long)
#endif
GBLDEF	char		**cmd_arg;
#ifdef __osf__
#pragma pointer_size (restore)
#endif

#ifndef __vax
GBLDEF	fnpc_area	fnpca;			/* $piece cache structure area */
#endif

#ifdef MUTEX_MSEM_WAKE
GBLDEF	volatile uint4	heartbeat_counter = 0;
#endif

/* DEFERRED EVENTS */
GBLDEF	int		dollar_zmaxtptime = 0;
GBLDEF	bool		licensed = TRUE;

#if defined(UNIX)
GBLDEF	volatile int4		num_deferred;
#elif defined(VMS)
GBLDEF	volatile short		num_deferred;
GBLDEF	int4 			lkid, lid;
GBLDEF	desblk			exi_blk;
GBLDEF	struct chf$signal_array	*tp_restart_fail_sig;
GBLDEF	boolean_t		tp_restart_fail_sig_used;
#else
# error "Unsupported Platform"
#endif

GBLDEF	volatile	int4		fast_lock_count = 0;	/* Used in wcs_stale */

/* REPLICATION RELATED GLOBALS */
GBLDEF gtmsource_options_t      gtmsource_options;

GBLDEF	unsigned char		*profstack_base, *profstack_top, *prof_msp, *profstack_warn;
GBLDEF	unsigned char		*prof_stackptr;
GBLDEF	boolean_t		is_tracing_on;
GBLDEF	stack_frame_prof	*prof_fp;
GBLDEF	void			(*tp_timeout_start_timer_ptr)(int4 tmout_sec) = tp_start_timer_dummy;
GBLDEF	void			(*tp_timeout_clear_ptr)(void) = tp_clear_timeout_dummy;
GBLDEF	void			(*tp_timeout_action_ptr)(void) = tp_timeout_action_dummy;
GBLDEF	void			(*ctrlc_handler_ptr)() = ctrlc_handler_dummy;
GBLDEF	int			(*op_open_ptr)(mval *v, mval *p, int t, mval *mspace) = op_open_dummy;
GBLDEF	void			(*unw_prof_frame_ptr)(void) = unw_prof_frame_dummy;
GBLDEF	boolean_t		mu_reorg_process = FALSE;
GBLDEF	gv_key			*gv_currkey_next_reorg;
GBLDEF	gv_namehead		*reorg_gv_target;

#ifdef UNIX
GBLDEF	struct sockaddr_un	gtmsecshr_sock_name;
GBLDEF	struct sockaddr_un	gtmsecshr_cli_sock_name;
GBLDEF	key_t			gtmsecshr_key;
#endif
GBLDEF	int			gtmsecshr_sockpath_len;
GBLDEF	int			gtmsecshr_cli_sockpath_len;
GBLDEF	mstr			gtmsecshr_pathname;
GBLDEF	int			server_start_tries;
GBLDEF	int			gtmsecshr_log_file;
GBLDEF	int			gtmsecshr_sockfd = -1;
GBLDEF	boolean_t		gtmsecshr_sock_init_done = FALSE;
GBLDEF	char			muext_code[MUEXT_MAX_TYPES][2] =
				{	{'0', '0'},
					{'0', '1'},
					{'0', '2'},
					{'0', '3'},
					{'0', '4'},
					{'0', '5'},
					{'0', '6'},
					{'0', '7'},
					{'0', '8'},
					{'0', '9'},
					{'1', '0'}
				};
GBLDEF	int			patch_is_fdmp;
GBLDEF	int			patch_fdmp_recs;
GBLDEF	boolean_t		horiz_growth = FALSE;
GBLDEF	int4			prev_first_off, prev_next_off;
				/* these two globals store the values of first_off and next_off in cse,
				 * when there is a blk split at index level. This is to permit rollback
				 * to intermediate states */
GBLDEF	boolean_t		lv_dupcheck = FALSE;
GBLDEF	sm_uc_ptr_t		min_mmseg;
GBLDEF	sm_uc_ptr_t		max_mmseg;
GBLDEF	mmseg			*mmseg_head;
GBLDEF	ua_list			*first_ua, *curr_ua;
GBLDEF	char			*update_array, *update_array_ptr;
GBLDEF	int			gv_fillfactor = 100,
				rc_set_fragment;       /* Contains offset within data at which data fragment starts */
GBLDEF	uint4			update_array_size = 0,
				cumul_update_array_size = 0;    /* the current total size of the update array */
GBLDEF	kill_set		*kill_set_tail;
GBLDEF	boolean_t		pool_init = FALSE;
GBLDEF	boolean_t		is_src_server = FALSE;
GBLDEF	boolean_t		is_rcvr_server = FALSE;
GBLDEF	jnl_format_buffer	*non_tp_jfb_ptr = NULL;
GBLDEF	unsigned char		*non_tp_jfb_buff_ptr;
GBLDEF	boolean_t		dse_running = FALSE;
GBLDEF	inctn_opcode_t		inctn_opcode = inctn_invalid_op;
GBLDEF	jnlpool_addrs		jnlpool;
GBLDEF	jnlpool_ctl_ptr_t	jnlpool_ctl;
GBLDEF	jnlpool_ctl_struct	temp_jnlpool_ctl_struct;
GBLDEF	jnlpool_ctl_ptr_t	temp_jnlpool_ctl = &temp_jnlpool_ctl_struct;
GBLDEF	sm_uc_ptr_t		jnldata_base;
GBLDEF	int4			jnlpool_shmid = INVALID_SHMID;
GBLDEF	recvpool_addrs		recvpool;
GBLDEF	int			recvpool_shmid = INVALID_SHMID;
GBLDEF	int			gtmsource_srv_count = 0;
GBLDEF	int			gtmrecv_srv_count = 0;

/* The following _in_prog counters are needed to prevent deadlocks while doing jnl-qio (timer & non-timer). */
GBLDEF	volatile int4		db_fsync_in_prog;
GBLDEF	volatile int4		jnl_qio_in_prog;
#ifdef UNIX
GBLDEF	gtmsiginfo_t		signal_info;
GBLDEF	boolean_t		mutex_salvaged;
#ifndef MUTEX_MSEM_WAKE
GBLDEF	int			mutex_sock_fd = -1;
GBLDEF	struct sockaddr_un	mutex_sock_address;
GBLDEF	struct sockaddr_un	mutex_wake_this_proc;
GBLDEF	int			mutex_wake_this_proc_len;
GBLDEF	int			mutex_wake_this_proc_prefix_len;
GBLDEF	fd_set			mutex_wait_on_descs;
#endif
#endif
GBLDEF	void			(*call_on_signal)();
GBLDEF	gtmImageName		gtmImageNames[n_image_types] =
{
#define IMAGE_TABLE_ENTRY(A,B)	{LIT_AND_LEN(B)},
#include "gtmimagetable.h"
#undef IMAGE_TABLE_ENTRY
};
GBLDEF	enum gtmImageTypes	image_type;	/* initialized at startup i.e. in dse.c, lke.c, gtm.c, mupip.c, gtmsecshr.c etc. */
GBLDEF	volatile boolean_t	semwt2long;

#ifdef UNIX
GBLDEF	parmblk_struct 		*param_list; /* call-in parameters block (defined in unix/fgncalsp.h)*/
GBLDEF	unsigned int		invocation_mode = MUMPS_COMPILE; /* how mumps has been invoked */
GBLDEF	char			cli_err_str[MAX_CLI_ERR_STR] = "";   /* Parse Error message buffer */
GBLDEF	char			*cli_err_str_ptr = NULL;
#endif

/* this array is indexed by file descriptor */
GBLDEF	boolean_t		*lseekIoInProgress_flags = (boolean_t *)0;

#if defined(UNIX)
/* Latch variable for Unix implementations. Used in SUN and HP */
GBLDEF	global_latch_t		defer_latch;
#endif

GBLDEF	int			num_additional_processors;
GBLDEF	int			gtm_errno = -1;		/* holds the errno (unix) in case of an rts_error */
GBLDEF	int4			error_condition = 0;
GBLDEF	global_tlvl_info	*global_tlvl_info_head;
GBLDEF	buddy_list		*global_tlvl_info_list;
GBLDEF	boolean_t		job_try_again;
GBLDEF	volatile int4		gtmMallocDepth;		/* Recursion indicator */
GBLDEF	d_socket_struct		*socket_pool;
GBLDEF	boolean_t		disable_sigcont = FALSE;
GBLDEF	boolean_t		mu_star_specified;

#ifndef VMS
GBLDEF	volatile int		suspend_status = NO_SUSPEND;
#endif

GBLDEF	gv_namehead		*reset_gv_target = INVALID_GV_TARGET;
GBLDEF	VSIG_ATOMIC_T		util_interrupt = 0;
GBLDEF	boolean_t		kip_incremented;
GBLDEF	boolean_t		need_kip_incr;
GBLDEF	int			merge_args = 0;
GBLDEF	merge_glvn_ptr		mglvnp = NULL;
GBLDEF	int			ztrap_form;
GBLDEF	boolean_t		ztrap_new;
GBLDEF	int4			wtfini_in_prog;
/* items for $piece stats */
#ifdef DEBUG
GBLDEF	int	c_miss;				/* cache misses (debug) */
GBLDEF	int	c_hit;				/* cache hits (debug) */
GBLDEF	int	c_small;			/* scanned small string brute force */
GBLDEF	int	c_small_pcs;			/* chars scanned by small scan */
GBLDEF	int	c_pskip;			/* number of pieces "skipped" */
GBLDEF	int	c_pscan;			/* number of pieces "scanned" */
GBLDEF	int	c_parscan;			/* number of partial scans (partial cache hits) */
GBLDEF	int	cs_miss;			/* cache misses (debug) */
GBLDEF	int	cs_hit;				/* cache hits (debug) */
GBLDEF	int	cs_small;			/* scanned small string brute force */
GBLDEF	int	cs_small_pcs;			/* chars scanned by small scan */
GBLDEF	int	cs_pskip;			/* number of pieces "skipped" */
GBLDEF	int	cs_pscan;			/* number of pieces "scanned" */
GBLDEF	int	cs_parscan;			/* number of partial scans (partial cache hits) */
GBLDEF	int	c_clear;			/* cleared due to (possible) value change */
GBLDEF	boolean_t	setp_work;
#endif
GBLDEF z_records	zbrk_recs = {NULL, NULL, NULL};

#ifdef UNIX
GBLDEF	ipcs_mesg	db_ipcs;		/* For requesting gtmsecshr to update ipc fields */
GBLDEF	gd_region	*ftok_sem_reg = NULL;	/* Last region for which ftok semaphore is grabbed */
GBLDEF	gd_region	*standalone_reg = NULL;	/* We have standalone access for this region */
#endif

#ifdef VMS
GBLDEF	uint4	check_channel_status = 0; /* stores the qio return status just before GTMASSERT in CHECK_CHANNEL_STATUS macro */
GBLDEF	uint4	check_channel_id = 0; 	/* stores the channel id just before a qio in a global variable for debugging purposes */
#endif

GBLDEF	boolean_t		write_after_image = FALSE;	/* true for after-image jnlrecord writing by recover/rollback */
GBLDEF	int			iott_write_error;
GBLDEF	boolean_t		recovery_success = FALSE; /* To Indicate successful recovery */
GBLDEF	int4			write_filter;
GBLDEF	int4			zdate_form = 0;
GBLDEF	boolean_t		need_no_standalone = FALSE;

GBLDEF	int4	zdir_form = ZDIR_FORM_FULLPATH; /* $ZDIR shows full path including DEVICE and DIRECTORY */
GBLDEF	mval	dollar_zdir = DEFINE_MVAL_LITERAL(MV_STR, 0, 0, 0, NULL, 0, 0);

GBLDEF	int * volatile		var_on_cstack_ptr = NULL; /* volatile pointer to int; volatile so that nothing gets optimized out */
GBLDEF	boolean_t		gtm_environment_init = FALSE;
GBLDEF	hashtab			*cw_stagnate = NULL;
GBLDEF	boolean_t		cw_stagnate_reinitialized = FALSE;

GBLDEF	uint4		pat_everything[] = { 0, 2, PATM_E, 1, 0, PAT_MAX_REPEAT, 0, PAT_MAX_REPEAT, 1 }; /* pattern = ".e" */
GBLDEF	uint4		sizeof_pat_everything = sizeof(pat_everything);

GBLDEF	uint4		*pattern_typemask;
GBLDEF	pattern		*pattern_list;
GBLDEF	pattern		*curr_pattern;

/* Standard MUMPS pattern-match table.
 * This table holds the current pattern-matching attributes of each ASCII character.
 * Bits 0..23 of each entry correspond with the pattern-match characters, A..X.
 */
GBLDEF pattern mumps_pattern = {
	(void *) 0,		/* flink */
	(void *) 0,		/* typemask */
	(void *) 0,		/* pat YZ name array */
	(void *) 0,		/* pat YZ name-length array */
	-1,			/* number of YZ patcodes */
	1,			/* namlen */
	{'M', '\0'}		/* name */
};

/* mapbit is used by pattab.c and patstr.c. Note that patstr.c uses only entries until PATM_X */
GBLDEF	readonly uint4	mapbit[] =
{
	PATM_A, PATM_B, PATM_C, PATM_D, PATM_E, PATM_F, PATM_G, PATM_H,
	PATM_I, PATM_J, PATM_K, PATM_L, PATM_M, PATM_N, PATM_O, PATM_P,
	PATM_Q, PATM_R, PATM_S, PATM_T, PATM_U, PATM_V, PATM_W, PATM_X,
	PATM_YZ1, PATM_YZ2, PATM_YZ3, PATM_YZ4
};

LITDEF	uint4	typemask[PATENTS] =
{
	PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C,
	PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C,
	PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C,
	PATM_C, PATM_C, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P,
	PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_N, PATM_N,
	PATM_N, PATM_N, PATM_N, PATM_N, PATM_N, PATM_N, PATM_N, PATM_N, PATM_P, PATM_P,
	PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U,
	PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U,
	PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U,
	PATM_U, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_L, PATM_L, PATM_L,
	PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L,
	PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L,
	PATM_L, PATM_L, PATM_L, PATM_P, PATM_P, PATM_P, PATM_P, PATM_C, PATM_C, PATM_C,
	PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C,
	PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C,
	PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C, PATM_C,
	PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P,
	PATM_L, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P,
	PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_P, PATM_L, PATM_P, PATM_P, PATM_P,
	PATM_P, PATM_P, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U,
	PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_P, PATM_U,
	PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U, PATM_U,
	PATM_U, PATM_U, PATM_P, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L,
	PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L,
	PATM_P, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L, PATM_L,
	PATM_L, PATM_L, PATM_L, PATM_L, PATM_P, PATM_C
};

GBLDEF	uint4		pat_allmaskbits;	/* universal set of valid pattern bit codes for currently active pattern table */

/* globals related to caching of pattern evaluation match result.
 * for a given <strptr, strlen, patptr, depth> tuple, we store the evaluation result.
 * in the above,
 *	<strptr, strlen>	uniquely identifies a substring of the input string.
 *	<patptr>		identifies the pattern atom that we are matching with
 *	<depth>			identifies the recursion depth of do_patalt() for this pattern atom ("repcnt" in code)
 * note that <depth> is a necessity in the above because the same alternation pattern atom can have different
 *	match or not-match status for the same input string depending on the repetition count usage of the pattern atom
 * after a series of thoughts on an efficient structure for storing pattern evaluation, finally arrived at a simple
 *	array of structures wherein for a given length (strlen) we have a fixed number of structures available.
 * we allocate an array of structures, say, 1024 structures.
 * this is a simple 1-1 mapping, wherein
 *	for length 0, the available structures are the first 32 structures of the array,
 *	for length 1, the available structures are the second 32 structures of the array.
 *	...
 *	for length 47, the available structures are the 47th 32 structures of the array.
 *	for length 48 and above, the available structures are all the remaining structures of the array.
 * whenever any new entry needs to be cached and there is no room among the available structures, we preempt the
 *	most unfrequently used cache entry (to do this we do keep a count of every entry's frequency of usage)
 * the assumption is that substrings of length > 48 (an arbitrary reasonable small number) won't be used
 *	so frequently so that they have lesser entries to fight for among themselves than lower values of length.
 * with the above caching in place, the program segment below took 15 seconds.
 * it was found that if the array size is increased to 16384 (as opposed to 1024 as above) and the available
 *	structures for each length increased proportionally (i.e. 16 times = 16*32 structures instead of 32 as above)
 *	the performance improved to the extent of taking 3 seconds.
 * but this raised an interesting question, that of "size" vs. "time" tradeoff.
 * with increasing array size, we get better "time" performance due to better caching.
 * but that has an overhead of increased "size" (memory) usage.
 * to arrive at a compromise, a dynamic algorithm emerged. the process will allocate a small array
 *	beginning at 1024 entries and grow to a max of 16384 entries as and when it deems the hit ratio is not good.
 * the array only grows, i.e. there is no downsizing algorithm at play.
 * the dynamic algorithm addresses to an extent both the "size" and "time" issues and finishes the below in 1 second.
 * #defines for the dynamic algorithm growth can be found in patcode.h
 */
GBLDEF	int4		curalt_depth = -1;				/* depth of alternation nesting */
GBLDEF	int4		do_patalt_calls[PTE_MAX_CURALT_DEPTH];		/* number of calls to do_patalt() */
GBLDEF	int4		do_patalt_hits[PTE_MAX_CURALT_DEPTH];		/* number of pte_csh hits in do_patalt() */
GBLDEF	int4		do_patalt_maxed_out[PTE_MAX_CURALT_DEPTH];	/* no. of pte_csh misses after maxing on allocation size */

GBLDEF	pte_csh		*pte_csh_array[PTE_MAX_CURALT_DEPTH];		/* pte_csh array (per curalt_depth) */
GBLDEF	int4		pte_csh_cur_size[PTE_MAX_CURALT_DEPTH];		/* current pte_csh size (per curalt_depth) */
GBLDEF	int4		pte_csh_alloc_size[PTE_MAX_CURALT_DEPTH];	/* current allocated pte_csh size (per curalt_depth) */
GBLDEF	int4		pte_csh_entries_per_len[PTE_MAX_CURALT_DEPTH];	/* current number of entries per len */
GBLDEF	int4		pte_csh_tail_count[PTE_MAX_CURALT_DEPTH];	/* count of non 1-1 corresponding pte_csh_array members */

GBLDEF	pte_csh		*cur_pte_csh_array;			/* copy of pte_csh_array corresponding to curalt_depth */
GBLDEF	int4		cur_pte_csh_size;			/* copy of pte_csh_cur_size corresponding to curalt_depth */
GBLDEF	int4		cur_pte_csh_entries_per_len;		/* copy of pte_csh_entries_per_len corresponding to curalt_depth */
GBLDEF	int4		cur_pte_csh_tail_count;			/* copy of pte_csh_tail_count corresponding to curalt_depth */

GBLDEF	readonly char	*before_image_lit[] = {"NOBEFORE_IMAGES", "BEFORE_IMAGES"};
GBLDEF	readonly char	*jnl_state_lit[]    = {"DISABLED", "OFF", "ON"};
GBLDEF	readonly char	*repl_state_lit[]   = {"OFF",      "ON"};

GBLDEF	boolean_t	crit_sleep_expired;		/* mutex.mar: signals that a timer waiting for crit has expired */
GBLDEF	uint4		crit_deadlock_check_cycle;	/* compared to csa->crit_check_cycle to determine if a given region
							   in a transaction legitimately has crit or not */
GBLDEF	node_local_ptr_t	locknl;		/* if non-NULL, indicates node-local of interest to the LOCK_HIST macro */
GBLDEF	boolean_t	in_mutex_deadlock_check;	/* if TRUE, mutex_deadlock_check() is part of our current C-stack trace */
/* $ECODE and $STACK related variables.
 * error_frame and skip_error_ret should ideally be part of dollar_ecode structure. since sr_avms/opp_ret.m64 uses these
 * global variables and it was felt risky changing it to access a member of a structure, they are kept as separate globals */
GBLDEF	stack_frame		*error_frame;		/* ptr to frame where last error occurred or was rethrown */
GBLDEF	boolean_t		skip_error_ret;		/* set to TRUE by golevel(), used and reset by op_unwind() */
GBLDEF	dollar_ecode_type	dollar_ecode;		/* structure containing $ECODE related information */
GBLDEF	dollar_stack_type	dollar_stack;		/* structure containing $STACK related information */
GBLDEF	unsigned char		*error_frame_save_mpc[DOLLAR_STACK_MAXINDEX];	/* save the mpc before resetting to error_ret()
										 * do this for 256 levels */
GBLDEF	boolean_t		ztrap_explicit_null;	/* whether $ZTRAP was explicitly set to NULL in the current frame */
GBLDEF	int4			gtm_object_size;	/* Size of entire gtm object for compiler use */
GBLDEF	int4			linkage_size;		/* Size of linkage section during compile */
GBLDEF	uint4			lnkrel_cnt;		/* number of entries in linkage Psect to relocate */
GBLDEF	boolean_t		disallow_forced_expansion, forced_expansion; /* Used in stringpool managment */
GBLDEF	jnl_fence_control	jnl_fence_ctl;
GBLDEF	jnl_process_vector	*prc_vec = NULL;		/* for current process */
GBLDEF	jnl_process_vector	*originator_prc_vec = NULL;	/* for client/originator */
LITDEF	char	*jrt_label[JRT_RECTYPES] =
{
#define JNL_TABLE_ENTRY(rectype, extract_rtn, label, update, fixed_size, is_replicated)	label,
#include "jnl_rec_table.h"
#undef JNL_TABLE_ENTRY
};
LITDEF	int	jrt_update[JRT_RECTYPES] =
{
#define JNL_TABLE_ENTRY(rectype, extract_rtn, label, update, fixed_size, is_replicated)	update,
#include "jnl_rec_table.h"
#undef JNL_TABLE_ENTRY
};
LITDEF	boolean_t	jrt_fixed_size[JRT_RECTYPES] =
{
#define JNL_TABLE_ENTRY(rectype, extract_rtn, label, update, fixed_size, is_replicated)	fixed_size,
#include "jnl_rec_table.h"
#undef JNL_TABLE_ENTRY
};
LITDEF	boolean_t	jrt_is_replicated[JRT_RECTYPES] =
{
#define JNL_TABLE_ENTRY(rectype, extract_rtn, label, update, fixed_size, is_replicated)	is_replicated,
#include "jnl_rec_table.h"
#undef JNL_TABLE_ENTRY
};
/* Change the initialization if struct_jrec_tcom in jnl.h changes */
GBLDEF	struct_jrec_tcom	tcom_record = {{JRT_TCOM, TCOM_RECLEN, 0, 0, 0},
					0, "", 0, {TCOM_RECLEN, JNL_REC_SUFFIX_CODE}};
GBLDEF 	jnl_gbls_t		jgbl;
GBLDEF short 		crash_count;
GBLDEF trans_num	start_tn;
GBLDEF cw_set_element	cw_set[CDB_CW_SET_SIZE];
GBLDEF unsigned char	cw_set_depth, cw_map_depth;
GBLDEF unsigned int	t_tries;
GBLDEF uint4		t_err;
GBLDEF int4		update_trans;	/* TRUE whenever a non-TP transaction needs to increment the database transaction number.
					 * usually TRUE if cw_set_depth/cw_map_depth of the current non-TP transaction is non-zero,
					 * but additionally TRUE in case of a redundant set in gvcst_put.c
					 * can take on a special value T_COMMIT_STARTED in t_end/tp_tend hence is not "boolean_t"
					 */
GBLDEF	boolean_t	mu_rndwn_file_dbjnl_flush;	/* to indicate standalone access is available to shared memory so
							 * wcs_recover() need not increment db curr_tn or write inctn record */

GBLDEF	boolean_t	is_uchar_wcs_code[] = 	/* uppercase failure codes that imply database cache related problem */
{	/* if any of the following failure codes are seen in the final retry, wc_blocked will be set to trigger cache recovery */
#define	CDB_SC_NUM_ENTRY(code, value)
#define CDB_SC_UCHAR_ENTRY(code, is_wcs_code, value)	is_wcs_code,
#define	CDB_SC_LCHAR_ENTRY(code, is_wcs_code, value)
#include "cdb_sc_table.h"
#undef CDB_SC_NUM_ENTRY
#undef CDB_SC_UCHAR_ENTRY
#undef CDB_SC_LCHAR_ENTRY
};

GBLDEF	boolean_t	is_lchar_wcs_code[] = 	/* lowercase failure codes that imply database cache related problem */
{	/* if any of the following failure codes are seen in the final retry, wc_blocked will be set to trigger cache recovery */
#define	CDB_SC_NUM_ENTRY(code, value)
#define CDB_SC_UCHAR_ENTRY(code, is_wcs_code, value)
#define	CDB_SC_LCHAR_ENTRY(code, is_wcs_code, value)	is_wcs_code,
#include "cdb_sc_table.h"
#undef CDB_SC_NUM_ENTRY
#undef CDB_SC_UCHAR_ENTRY
#undef CDB_SC_LCHAR_ENTRY
};

GBLDEF	mval	last_fnquery_return_varname;			/* Return value of last $QUERY (on stringpool) (varname) */
GBLDEF	mval	last_fnquery_return_sub[MAX_LVSUBSCRIPTS];	/* .. (subscripts) */
GBLDEF	int	last_fnquery_return_subcnt	;		/* .. (count of subscripts) */

GBLDEF	boolean_t	gvdupsetnoop = FALSE;	/* if TRUE, duplicate SETs do not change GDS block (and therefore no PBLK journal
						 * records will be written) although the database transaction number will be
						 * incremented and logical SET journal records will be written.
						 */
GBLDEF boolean_t	gtm_fullblockwrites;	/* Do full (not partial) database block writes T/F */
UNIX_ONLY(GBLDEF int4	gtm_shmflags;)		/* Extra flags for shmat */
#ifdef VMS
GBLDEF	uint4	gtm_memory_noaccess_defined;	/* count of the number of GTM_MEMORY_NOACCESS_ADDR logicals which are defined */
GBLDEF	uint4	gtm_memory_noaccess[GTM_MEMORY_NOACCESS_COUNT];	/* see VMS gtm_env_init_sp.c */
#endif

#ifdef DEBUG
GBLDEF	boolean_t	in_wcs_recover = FALSE;	/* TRUE if in wcs_recover(), used by bt_put() */
#endif
