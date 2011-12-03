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

#include <stddef.h>		/* for offsetof macro */

#include "gtm_string.h"

#include "cdb_sc.h"
#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "gdsblk.h"
#include "gdskill.h"
#include "gdscc.h"
#include "min_max.h"		/* needed for gdsblkops.h */
#include "gdsblkops.h"
#include "filestruct.h"
#include "iosp.h"
#include "jnl.h"
#include "hashtab.h"		/* needed for tp.h */
#include "buddy_list.h"		/* needed for tp.h */
#include "tp.h"
#include "gtm_stdlib.h"		/* for ATOI */
#include "cryptdef.h"
#include "mlkdef.h"
#include "error.h"
#include "gt_timer.h"
#include "gtmimagename.h"
#include "trans_log_name.h"
#include "gtm_logicals.h"
#include "gvcst_init.h"
#include "dbfilop.h"
#include "gvcst_init_sysops.h"
#include "set_num_additional_processors.h"
#include "have_crit.h"
#include "t_retry.h"
#include "gvcst_tp_init.h"
#include "dpgbldir.h"
#include "longset.h"		/* needed for cws_insert.h */
#include "cws_insert.h"		/* for CWS_INIT macro */

GBLREF	gd_region		*gv_cur_region, *db_init_region;
GBLREF	sgmnt_data_ptr_t	cs_data;
GBLREF	sgmnt_addrs		*cs_addrs;
GBLREF	boolean_t		gtcm_connection;
GBLREF	bool			licensed;
GBLREF	int4			lkid;
GBLREF	char			*update_array, *update_array_ptr;
GBLREF	uint4			update_array_size, cumul_update_array_size;
GBLREF	ua_list			*first_ua, *curr_ua;
GBLREF	short			crash_count, dollar_tlevel;
GBLREF	jnl_format_buffer	*non_tp_jfb_ptr;
GBLREF	unsigned char		*non_tp_jfb_buff_ptr;
GBLREF	boolean_t		mupip_jnl_recover;
GBLREF	buddy_list		*global_tlvl_info_list;
GBLREF	enum gtmImageTypes	image_type;
GBLREF	int			tprestart_syslog_limit;
GBLREF	int			tprestart_syslog_delta;
GBLREF	tp_region		*tp_reg_free_list;	/* Ptr to list of tp_regions that are unused */
GBLREF	tp_region		*tp_reg_list;		/* Ptr to list of tp_regions for this transaction */
GBLREF	unsigned int		t_tries;
GBLREF	hashtab			*cw_stagnate;
GBLREF	struct_jrec_tcom	tcom_record;
GBLREF	boolean_t		tp_in_use;

LITREF char			gtm_release_name[];
LITREF int4			gtm_release_name_len;

void	assert_jrec_member_offsets(void)
{
	assert(JNL_HDR_LEN % DISK_BLOCK_SIZE == 0);
	assert(JNL_HDR_LEN == JNL_FILE_FIRST_RECORD);
	assert(DISK_BLOCK_SIZE >= PINI_RECLEN + EPOCH_RECLEN + PFIN_RECLEN + EOF_RECLEN);
	/* Following assert is for JNL_FILE_TAIL_PRESERVE macro in tp.h */
	assert(PINI_RECLEN >= EPOCH_RECLEN && PINI_RECLEN >= PFIN_RECLEN && PINI_RECLEN >= EOF_RECLEN);
	assert(sizeof(jnl_str_len_t) == sizeof(uint4));
	/* since time in jnl record is a uint4, and since JNL_SHORT_TIME expects time_t, we better ensure they are same.
	 * A change in the size of time_t would mean a redesign of the fields.  */
	assert(sizeof(time_t) == sizeof(uint4));
	/* Make sure all jnl_seqno fields start at same offset. mur_output_record and others rely on this. */
	assert(offsetof(struct_jrec_null, jnl_seqno) == offsetof(struct_jrec_upd, token_seq.jnl_seqno));
	assert(offsetof(struct_jrec_null, jnl_seqno) == offsetof(struct_jrec_ztp_upd, jnl_seqno));
	assert(offsetof(struct_jrec_null, jnl_seqno) == offsetof(struct_jrec_epoch, jnl_seqno));
	assert(offsetof(struct_jrec_null, jnl_seqno) == offsetof(struct_jrec_eof, jnl_seqno));
	assert(offsetof(struct_jrec_null, jnl_seqno) == offsetof(struct_jrec_tcom, token_seq.jnl_seqno));
	assert(offsetof(struct_jrec_null, jnl_seqno) == offsetof(struct_jrec_ztcom, jnl_seqno));

	assert(offsetof(struct_jrec_ztcom, token) == offsetof(struct_jrec_ztp_upd, token));
	/* Make sure all jnl_seqno and token fields start at 8-byte boundary */
	assert(offsetof(struct_jrec_upd, token_seq.jnl_seqno) ==
		(ROUND_UP(offsetof(struct_jrec_upd, token_seq.jnl_seqno), sizeof(seq_num))));
	assert(offsetof(struct_jrec_ztp_upd, jnl_seqno) ==
		(ROUND_UP(offsetof(struct_jrec_ztp_upd, jnl_seqno), sizeof(seq_num))));
	assert(offsetof(struct_jrec_tcom, token_seq.jnl_seqno) ==
		(ROUND_UP(offsetof(struct_jrec_tcom, token_seq.jnl_seqno), sizeof(seq_num))));
	assert(offsetof(struct_jrec_ztcom, jnl_seqno) ==
		(ROUND_UP(offsetof(struct_jrec_ztcom, jnl_seqno), sizeof(seq_num))));
	assert(offsetof(struct_jrec_null, jnl_seqno) ==
		(ROUND_UP(offsetof(struct_jrec_null, jnl_seqno), sizeof(seq_num))));
	assert(offsetof(struct_jrec_epoch, jnl_seqno) ==
		(ROUND_UP(offsetof(struct_jrec_epoch, jnl_seqno), sizeof(seq_num))));
	assert(offsetof(struct_jrec_eof, jnl_seqno) ==
		(ROUND_UP(offsetof(struct_jrec_eof, jnl_seqno), sizeof(seq_num))));
	/* All fixed size records must be multiple of 8-byte */
	assert(TCOM_RECLEN == (ROUND_UP(sizeof(struct_jrec_tcom), JNL_REC_START_BNDRY)));
	assert(ZTCOM_RECLEN == (ROUND_UP(sizeof(struct_jrec_ztcom), JNL_REC_START_BNDRY)));
	assert(INCTN_RECLEN == (ROUND_UP(sizeof(struct_jrec_inctn), JNL_REC_START_BNDRY)));
	assert(PINI_RECLEN == (ROUND_UP(sizeof(struct_jrec_pini), JNL_REC_START_BNDRY)));
	assert(PFIN_RECLEN == (ROUND_UP(sizeof(struct_jrec_pfin), JNL_REC_START_BNDRY)));
	assert(NULL_RECLEN == (ROUND_UP(sizeof(struct_jrec_null), JNL_REC_START_BNDRY)));
	assert(EPOCH_RECLEN == (ROUND_UP(sizeof(struct_jrec_epoch), JNL_REC_START_BNDRY)));
	assert(EOF_RECLEN == (ROUND_UP(sizeof(struct_jrec_eof), JNL_REC_START_BNDRY)));
	/* Assumption about the structures in code */
	assert(0 == MIN_ALIGN_RECLEN % JNL_REC_START_BNDRY);
	assert(sizeof(uint4) == sizeof(jrec_suffix));
	assert((MAX_JNL_REC_SIZE - MAX_LOGI_JNL_REC_SIZE) > MIN_PBLK_RECLEN);
	assert((DISK_BLOCK_SIZE * JNL_DEF_ALIGNSIZE) >= MAX_JNL_REC_SIZE);/* default alignsize supports max jnl record length */
	assert(MAX_DB_BLK_SIZE < MAX_JNL_REC_SIZE);	/* Ensure a PBLK record can accommodate a full GDS block */
	assert(tcom_record.prefix.forwptr == tcom_record.suffix.backptr);
	assert(TCOM_RECLEN == tcom_record.suffix.backptr);
}

void gvcst_init (gd_region *greg)
{
	sgmnt_addrs		*csa, *prevcsa, *regcsa;
	sgmnt_data_ptr_t	csd, temp_cs_data;
	char			cs_data_buff[ROUND_UP(sizeof(sgmnt_data), DISK_BLOCK_SIZE)];
	uint4			segment_update_array_size;
	file_control		*fc;
	gd_region		*prev_reg, *reg_top;
#ifdef DEBUG
	cache_rec_ptr_t		cr;
	bt_rec_ptr_t		bt;
	blk_ident		tmp_blk;
#endif
	mstr			log_nam, trans_log_nam;
	char			trans_buff[MAX_FN_LEN+1];
	static int4		first_time = TRUE;
	char			now_running[MAX_REL_NAME];
	unique_file_id		*greg_fid, *reg_fid;
	gd_addr			*addr_ptr;
	tp_region		*tr;
	int4			prev_index;
	ua_list			*tmp_ua;

	error_def (ERR_DBFLCORRP);
	error_def (ERR_DBCREINCOMP);
	error_def (ERR_DBNOTGDS);
	error_def (ERR_BADDBVER);
	error_def (ERR_VERMISMATCH);

	CWS_INIT;	/* initialize the cw_stagnate hash-table */

	/* we shouldn't have crit on any region unless we are in TP and in the final retry or we are in
	 * mupip_set_journal trying to switch journals across all regions. Currently, there is no fine-granular
	 * checking for mupip_set_journal, hence a coarse MUPIP_IMAGE check for image_type
	 */
	assert(dollar_tlevel && (CDB_STAGNATE <= t_tries) || MUPIP_IMAGE == image_type || (0 == have_crit(CRIT_HAVE_ANY_REG)));
	if ((0 < dollar_tlevel) && (0 != have_crit(CRIT_HAVE_ANY_REG)))
	{	/* to avoid deadlocks with currently holding crits and the DLM lock request to be done in db_init(),
		 * we should insert this region in the tp_reg_list and tp_restart should do the gvcst_init after
		 * having released crit on all regions.
		 */
		insert_region(greg, &tp_reg_list, &tp_reg_free_list, sizeof(tp_region));
		t_retry(cdb_sc_needcrit);
		assert(FALSE);	/* we should never reach here since t_retry should have unwound the M-stack and restarted the TP */
	}
	/* check the header design assumptions */
	assert(sizeof(th_rec) == (sizeof(bt_rec) - sizeof(bt->blkque)));
	assert(sizeof(cache_rec) == (sizeof(cache_state_rec) + sizeof(cr->blkque)));
	DEBUG_ONLY(assert_jrec_member_offsets();)
        set_num_additional_processors();

	DEBUG_ONLY(
		/* Note that the "block" member in the blk_ident structure in gdskill.h has 26 bits.
		 * Currently, the maximum number of blocks is 2**26. If ever this increases, something
		 * has to be correspondingly done to the "block" member to increase its capacity.
		 * The following assert checks that we always have space in the "block" member
		 * to represent a GDS block number.
		 */
		tmp_blk.block = -1;
		assert(MAXTOTALBLKS - 1 <= tmp_blk.block);
	)
	if (TRUE == first_time)
	{
		log_nam.addr = GTM_TPRESTART_LOG_LIMIT;
		log_nam.len = STR_LIT_LEN(GTM_TPRESTART_LOG_LIMIT);
		if (trans_log_name(&log_nam, &trans_log_nam, trans_buff) == SS_NORMAL)
		{
			tprestart_syslog_limit = ATOI(trans_log_nam.addr);
			if (0 > tprestart_syslog_limit)
				tprestart_syslog_limit = 0;
		}
		log_nam.addr = GTM_TPRESTART_LOG_DELTA;
		log_nam.len = STR_LIT_LEN(GTM_TPRESTART_LOG_DELTA);
		if (trans_log_name(&log_nam, &trans_log_nam, trans_buff) == SS_NORMAL)
		{
			tprestart_syslog_delta = ATOI(trans_log_nam.addr);
			if (0 > tprestart_syslog_delta)
				tprestart_syslog_delta = MAXPOSINT4;
		}
		first_time = FALSE;
	}
	if ((prev_reg = dbfilopn(greg)) != greg)
	{
		if (NULL == prev_reg || (gd_region *)-1 == prev_reg) /* (gd_region *)-1 == prev_reg => cm region open attempted */
			return;
		greg->dyn.addr->file_cntl = prev_reg->dyn.addr->file_cntl;
		memcpy(greg->dyn.addr->fname, prev_reg->dyn.addr->fname, prev_reg->dyn.addr->fname_len);
		greg->dyn.addr->fname_len = prev_reg->dyn.addr->fname_len;
		csa = (sgmnt_addrs *)&FILE_INFO(greg)->s_addrs;
		csd = csa->hdr;
		greg->max_rec_size = csd->max_rec_size;
		greg->max_key_size = csd->max_key_size;
	 	greg->null_subs = csd->null_subs;
		greg->jnl_state = csd->jnl_state;
		greg->jnl_file_len = csd->jnl_file_len;		/* journal file name length */
		memcpy(greg->jnl_file_name, csd->jnl_file_name, greg->jnl_file_len);	/* journal file name */
		greg->jnl_alq = csd->jnl_alq;
		greg->jnl_deq = csd->jnl_deq;
		greg->jnl_buffer_size = csd->jnl_buffer_size;
		greg->jnl_before_image = csd->jnl_before_image;
		greg->open = TRUE;
		greg->opening = FALSE;
		greg->was_open = TRUE;
		return;
	}
	greg->was_open = FALSE;
	csa = (sgmnt_addrs *)&FILE_INFO(greg)->s_addrs;

#ifdef	NOLICENSE
	licensed= TRUE ;
#else
	CRYPT_CHKSYSTEM ;
#endif
	db_init_region = greg;	/* initialized for dbinit_ch */
	csa->hdr = NULL;
        csa->nl = NULL;
        csa->jnl = NULL;
	csa->persistent_freeze = FALSE;	/* want secshr_db_clnup() to clear an incomplete freeze/unfreeze codepath */
	UNIX_ONLY(
		FILE_INFO(greg)->semid = INVALID_SEMID;
		FILE_INFO(greg)->shmid = INVALID_SHMID;
		FILE_INFO(greg)->sem_ctime = 0;
		FILE_INFO(greg)->shm_ctime = 0;
		FILE_INFO(greg)->ftok_semid = INVALID_SEMID;
	)
	VMS_ONLY(
		csa->db_addrs[0] = csa->db_addrs[1] = NULL;
		csa->lock_addrs[0] = csa->lock_addrs[1] = NULL;
	)
	UNSUPPORTED_PLATFORM_CHECK;
        ESTABLISH(dbinit_ch);

	temp_cs_data = (sgmnt_data_ptr_t)cs_data_buff;
	fc = greg->dyn.addr->file_cntl;
	fc->file_type = greg->dyn.addr->acc_meth;
	fc->op = FC_READ;
	fc->op_buff = (sm_uc_ptr_t)temp_cs_data;
	fc->op_len = sizeof(*temp_cs_data);
	fc->op_pos = 1;
	dbfilop(fc);
	if (memcmp(temp_cs_data->label, GDS_LABEL, GDS_LABEL_SZ - 1))
	{
		if (memcmp(temp_cs_data->label, GDS_LABEL, GDS_LABEL_SZ - 3))
			rts_error(VARLSTCNT(4) ERR_DBNOTGDS, 2, DB_LEN_STR(greg));
		else
			rts_error(VARLSTCNT(4) ERR_BADDBVER, 2, DB_LEN_STR(greg));
	}
	/* Set the following values to sane values for recovery/rollback */
	if (mupip_jnl_recover)
	{
		temp_cs_data->createinprogress = FALSE;
		temp_cs_data->freeze = 0;
		temp_cs_data->image_count = 0;
		temp_cs_data->owner_node = 0;
	}
	if (temp_cs_data->createinprogress)
		rts_error(VARLSTCNT(4) ERR_DBCREINCOMP, 2, DB_LEN_STR(greg));
	if (temp_cs_data->file_corrupt && !mupip_jnl_recover)
		rts_error(VARLSTCNT(4) ERR_DBFLCORRP, 2, DB_LEN_STR(greg));
	assert(greg->dyn.addr->acc_meth != dba_cm);
	if (greg->dyn.addr->acc_meth != temp_cs_data->acc_meth)
		greg->dyn.addr->acc_meth = temp_cs_data->acc_meth;

/* Here's the shared memory layout:
 *
 * low address
 *
 * both
 *	segment_data
 *	(file_header)
 *	MM_BLOCK
 *	(master_map)
 *	TH_BLOCK
 * BG
 *	bt_header
 *	(bt_buckets * bt_rec)
 *	th_base (sizeof(que_ent) into an odd bt_rec)
 *	bt_base
 *	(n_bts * bt_rec)
 *	LOCK_BLOCK (lock_space)
 *	(lock_space_size)
 *	cs_addrs->acc_meth.bg.cache_state
 *	(cache_que_heads)
 *	(bt_buckets * cache_rec)
 *	(n_bts * cache_rec)
 *	critical
 *	(mutex_struct)
 *	nl
 *	(node_local)
 *	[jnl_name
 *	jnl_buffer]
 * MM
 *	file contents
 *	LOCK_BLOCK (lock_space)
 *	(lock_space_size)
 *	cs_addrs->acc_meth.mm.mmblk_state
 *	(mmblk_que_heads)
 *	(bt_buckets * mmblk_rec)
 *	(n_bts * mmblk_rec)
 *	critical
 *	(mutex_struct)
 *	nl
 *	(node_local)
 *	[jnl_name
 *	jnl_buffer]
 * high address
 */
 	/* Ensure first 3 members (upto now_running) of node_local are at the same offset for any version.
	 * This is so that the VERMISMATCH error can be successfully detected in db_init/mu_rndwn_file
	 *	and so that the db-file-name can be successfully obtained from orphaned shm by mu_rndwn_all.
	 */
 	assert(offsetof(node_local, label[0]) == 0);
	assert(offsetof(node_local, fname[0]) == GDS_LABEL_SZ);
	assert(offsetof(node_local, now_running[0]) == GDS_LABEL_SZ + MAX_FN_LEN + 1);
	assert(sizeof(csa->nl->now_running) == MAX_REL_NAME);
	db_init(greg, temp_cs_data);
	crash_count = csa->critical->crashcnt;

	csd = csa->hdr;
	if (memcmp(csa->nl->now_running, gtm_release_name, gtm_release_name_len + 1))
	{	/* Copy csa->nl->now_running into a local variable before passing to rts_error() due to the following issue.
		 * In VMS, a call to rts_error() copies only the error message and its arguments (as pointers) and
		 *  transfers control to the topmost condition handler which is dbinit_ch() in this case. dbinit_ch()
		 *  does a PRN_ERROR only for SUCCESS/INFO (VERMISMATCH is neither of them) and in addition
		 *  nullifies csa->nl as part of its condition handling. It then transfers control to the next level condition
		 *  handler which does a PRN_ERROR but at that point in time, the parameter csa->nl->now_running is no longer
		 *  accessible and hence no parameter substitution occurs (i.e. the error message gets displayed with plain !ADs).
		 * In UNIX, this is not an issue since the first call to rts_error() does the error message
		 *  construction before handing control to the topmost condition handler. But it does not hurt to do the copy.
		 */
		assert(strlen(csa->nl->now_running) < sizeof(now_running));
		memcpy(now_running, csa->nl->now_running, sizeof(now_running));
		now_running[sizeof(now_running) - 1] = '\0';	/* protection against bad values of csa->nl->now_running */
		rts_error(VARLSTCNT(8) ERR_VERMISMATCH & ~SEV_MSK | ((DSE_IMAGE != image_type) ? ERROR : INFO), 6,
			DB_LEN_STR(greg), gtm_release_name_len, gtm_release_name, LEN_AND_STR(now_running));
	}
	/* set csd and fill in selected fields */
	switch (greg->dyn.addr->acc_meth)
	{
	case dba_mm:
		csa->acc_meth.mm.base_addr = (sm_uc_ptr_t)((sm_ulong_t)csd + (int)(csd->start_vbn - 1) * DISK_BLOCK_SIZE);
		break;
	case dba_bg:
		csa->clustered = csd->clustered;
		db_csh_ini(csa);
		break;
	default:
		GTMASSERT;
	}
	db_common_init(greg, csa, csd);	/* do initialization common to db_init() and mu_rndwn_file() */
	/* Compute the maximum journal space requirements for a PBLK (including possible ALIGN record).
	 * Use this variable in the TOTAL_TPJNL_REC_SIZE and TOTAL_NONTP_JNL_REC_SIZE macros instead of recomputing.
	 */
	csa->pblk_align_jrecsize = MIN_PBLK_RECLEN + csd->blk_size + MIN_ALIGN_RECLEN;
	segment_update_array_size = UA_SIZE(csd);

	if (first_ua == NULL)
	{	/* first open of first database - establish an update array system */
		assert(update_array == NULL);
		assert(update_array_ptr == NULL);
		assert(update_array_size == 0);
		tmp_ua = (ua_list *)malloc(sizeof(ua_list));
		memset(tmp_ua, 0, sizeof(ua_list));	/* initialize tmp_ua->update_array and tmp_ua->next_ua to NULL */
		tmp_ua->update_array = (char *)malloc(segment_update_array_size);
		tmp_ua->update_array_size = segment_update_array_size;
		/* assign global variables only after malloc() succeeds */
		update_array_size = cumul_update_array_size = segment_update_array_size;
		update_array = update_array_ptr = tmp_ua->update_array;
		first_ua = curr_ua = tmp_ua;
	} else
	{	/* there's already an update_array system in place */
		assert(update_array != NULL);
		assert(update_array_size != 0);
		if (!dollar_tlevel && segment_update_array_size > first_ua->update_array_size)
		{
			/* no transaction in progress and the current array is too small - replace it */
			assert(first_ua->update_array == update_array);
			assert(first_ua->update_array_size == update_array_size);
			assert(first_ua->next_ua == NULL);
			tmp_ua = first_ua;
			first_ua = curr_ua = NULL;
			free(update_array);
			tmp_ua->update_array = update_array = update_array_ptr = NULL;
			tmp_ua->update_array = (char *)malloc(segment_update_array_size);
			tmp_ua->update_array_size = segment_update_array_size;
			/* assign global variables only after malloc() succeeds */
			update_array_size = cumul_update_array_size = segment_update_array_size;
			update_array = update_array_ptr = tmp_ua->update_array;
			first_ua = curr_ua = tmp_ua;
		}
	}
	assert(global_tlvl_info_list || !csa->sgm_info_ptr);
	if (JNL_ALLOWED(csa))
	{
		if (NULL == non_tp_jfb_ptr)
		{
			non_tp_jfb_ptr = (jnl_format_buffer *)malloc(sizeof(jnl_format_buffer));
			non_tp_jfb_buff_ptr =  (unsigned char *)malloc(MAX_JNL_REC_SIZE);
			non_tp_jfb_ptr->buff = (char *) non_tp_jfb_buff_ptr;
			non_tp_jfb_ptr->record_size = 0;	/* initialize it to 0 since TOTAL_NONTPJNL_REC_SIZE macro uses it */
		}
		/* csa->min_total_tpjnl_rec_size represents the minimum journal buffer space needed for a TP transaction.
		 * It is a conservative estimate assuming that one ALIGN record and one PINI record will be written for
		 *	one set of fixed size jnl records written.
		 * si->total_jnl_rec_size is initialized/reinitialized  to this value here and in tp_clean_up().
		 * The purpose of this field is to avoid recomputation of the variable in tp_clean_up().
		 * In addition to this, space requirements for whatever journal records get formatted as part of
		 *	jnl_format() need to be taken into account.
		 *	This is done in jnl_format() where si->total_jnl_rec_size is appropriately incremented.
		 */
		csa->min_total_tpjnl_rec_size = PINI_RECLEN + TCOM_RECLEN + MIN_ALIGN_RECLEN;
		/* Similarly csa->min_total_nontpjnl_rec_size represents the minimum journal buffer space needed
		 *	for a non-TP transaction.
		 * It is a conservative estimate assuming that one ALIGN record and one PINI record will be written for
		 *	one set of fixed size jnl records written.
		 */
		csa->min_total_nontpjnl_rec_size = PINI_RECLEN + MIN_ALIGN_RECLEN;
	}
	if (tp_in_use || GTM_IMAGE != image_type)
		gvcst_tp_init(greg);	/* Initialize TP structures, else postpone till TP is used (only if GTM) */
	if (!global_tlvl_info_list)
	{
		global_tlvl_info_list = (buddy_list *)malloc(sizeof(buddy_list));
		initialize_list(global_tlvl_info_list, sizeof(global_tlvl_info), GBL_TLVL_INFO_LIST_INIT_ALLOC);
	}
	greg->open = TRUE;
	greg->opening = FALSE;
	if ((dba_bg == greg->dyn.addr->acc_meth) || (dba_mm == greg->dyn.addr->acc_meth))
	{
		/* Determine fid_index of current region's file_id across sorted file_ids of all regions open until now.
		 * All regions which have a file_id lesser than that of current region will have no change to their fid_index
		 * All regions which have a file_id greater than that of current region will have their fid_index incremented by 1
		 * The fid_index determination algorithm below has an optimization in that if the current region's file_id is
		 * determined to be greater than a that of a particular region, then all regions whose fid_index is lesser
		 * than that particular region's fid_index are guaranteed to have a lesser file_id than the current region
		 * so we do not compare those against the current region's file_id.
		 * Note that the sorting is done only on DB/MM regions. GT.CM/DDP regions should not be part of TP transactions,
		 * hence they will not be sorted.
		 */
		prevcsa = NULL;
		greg_fid = &(csa->nl->unique_id);
		for (addr_ptr = get_next_gdr(NULL); NULL != addr_ptr; addr_ptr = get_next_gdr(addr_ptr))
		{
			for (prev_reg = addr_ptr->regions, reg_top = prev_reg + addr_ptr->n_regions; prev_reg < reg_top; prev_reg++)
			{
				if ((!prev_reg->open) || (greg == prev_reg))
					continue;
				/* Only BG/MM regions can be involved in TP transactions, do not sort GT.CM or DDP regions */
				if (!((dba_bg == prev_reg->dyn.addr->acc_meth) || (dba_mm == prev_reg->dyn.addr->acc_meth)))
					continue;
				regcsa = (sgmnt_addrs *)&FILE_INFO(prev_reg)->s_addrs;
				if ((NULL != prevcsa) && (regcsa->fid_index < prevcsa->fid_index))
					continue;
				reg_fid = &((regcsa)->nl->unique_id);
				VMS_ONLY(if (0 < memcmp(&(greg_fid->file_id), (char *)&(reg_fid->file_id), sizeof(gd_id))))
				UNIX_ONLY(if (0 < gdid_cmp(&(greg_fid->uid), &(reg_fid->uid))))
				{
					if (NULL == prevcsa || regcsa->fid_index > prevcsa->fid_index)
						prevcsa = regcsa;
				} else
					regcsa->fid_index++;
			}
		}
		if (NULL == prevcsa)
			csa->fid_index = 1;
		else
			csa->fid_index = prevcsa->fid_index + 1;
		/* Also update tp_reg_list fid_index's as insert_region relies on it */
		DEBUG_ONLY(prev_index = 0;)
		for (tr = tp_reg_list; NULL != tr; tr = tr->fPtr)
		{
			tr->file.fid_index = (&FILE_INFO(tr->reg)->s_addrs)->fid_index;
			assert(prev_index < tr->file.fid_index);
			DEBUG_ONLY(prev_index = tr->file.fid_index);
		}
	}
	REVERT;
	return;
}
