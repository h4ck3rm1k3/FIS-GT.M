// all the missing functions go here for now
#include "mdef.h"

#include "gtm_string.h"
#include "gtm_unistd.h"
#include "gtm_stdlib.h"		/* for exit() */

#include <signal.h>
#include <netinet/in.h>

#include "gtm_stdio.h"
#include "error.h"

#include "types.h"
#include "../sr_port/types.h"

#include "gtmsiginfo.h"
#include "gtmimagename.h"
#include "gt_timer.h"
#include "send_msg.h"
#include "generic_signal_handler.h"
#include "gtmmsg.h"
#include "have_crit.h"


//void merrors_ctl () { }
//void pow () { }
err_ctl			merrors_ctl;
void CMERR_CMINTQUE () { }
void GET_RTNHDR_ADDR () { }
void LABADDR_PV_OFF () { }
void RTNHDR_PV_OFF () { }
void VALID_CALLING_SEQUENCE () { }
void VAR_START_HACK () { }
void aio_cancel64 () { }
void aio_error64 () { }
void aio_read64 () { }
void aio_return64 () { }
void aswp () { }
void authenticate () { }
void blkdlist () { }
void call_dm () { }
void callg () { }
void ci_restart () { }
void cli_get_string_token () { }
void cli_gettoken () { }
void cli_is_assign () { }
void cli_is_dcm () { }
void cli_is_hex () { }
void cli_is_qualif () { }
void cli_lex_in_ptr () { }
void cli_lex_setup () { }
void cli_look_next_string_token () { }
void cli_look_next_token () { }
void cli_str_setup () { }
void cli_strupper () { }
void cli_token_buf () { }
void cmerrors_ctl () { }
void cmierrors_ctl () { }
void comp_linkages () { }
void compswap () { }
void conn_timeout () { }
void curr_entry () { }
//void dlclose () { }
//void dlerror () { }
//void dlopen () { }
//void dlsym () { }
void dm_start () { }
void follow () { }
void gdeerrors_ctl () { }
void gtcm_action_pending () { }
void gtcm_ast_avail () { }
void gtcm_exi_condition () { }
void gtcm_exi_handler () { }
void gtcm_gnp_server_log () { }
void gtcm_init_ast () { }
void gtcm_int_unpack () { }
void gtcm_jnl_switched () { }
void gtcm_link_accept () { }
void gtcm_ltime () { }
void gtcm_neterr () { }
void gtcm_read_ast () { }
void gtcm_remove_from_action_queue () { }
void gtcm_shutdown_ast () { }
void gtcm_stime () { }
void gtcm_write_ast () { }
void gtcmd_cst_init () { }
void gtcml_chkreg () { }
void gtcmtr_bufflush () { }
void gtcmtr_data () { }
void gtcmtr_get () { }
void gtcmtr_initproc () { }
void gtcmtr_initreg () { }
void gtcmtr_kill () { }
void gtcmtr_lkacquire () { }
void gtcmtr_lkcanall () { }
void gtcmtr_lkcancel () { }
void gtcmtr_lkdelete () { }
void gtcmtr_lkreqimmed () { }
void gtcmtr_lkreqnode () { }
void gtcmtr_lkrequest () { }
void gtcmtr_lkresume () { }
void gtcmtr_lksuspend () { }
void gtcmtr_order () { }
void gtcmtr_put () { }
void gtcmtr_query () { }
void gtcmtr_terminate () { }
void gtcmtr_zprevious () { }
void gtcmtr_zwithdraw () { }
void gtm_ret_code () { }
void gvcmy_close () { }
void history () { }
void jsb_action () { }
void make_mode () { }
void mint2mval () { }
void mum_tstart () { }
void mutex_lockr () { }
void mval2bool () { }
void mval2mint () { }
void mval2num () { }
void omi_brecv () { }
void omi_bsent () { }
void omi_conns () { }
void omi_debug () { }
void omi_errno () { }
void omi_exitp () { }
void omi_nerrs () { }
void omi_nxact () { }
void omi_nxact2 () { }
void omi_pid () { }
void omi_pkdbg () { }
void omi_pklog () { }
void omi_pklog_addr () { }
void omi_service () { }
void one_conn_per_inaddr () { }
void op_callb () { }
void op_calll () { }
void op_callspb () { }
void op_callspl () { }
void op_callspw () { }
void op_callw () { }
void op_contain () { }
void op_currtn () { }
void op_equ () { }
void op_equnul () { }
void op_exfun () { }
void op_exfunret () { }
void op_extcall () { }
void op_extexfun () { }
void op_extjmp () { }
void op_fetchintrrpt () { }
void op_fnextract () { }
void op_fnget () { }
void op_fnlength () { }
void op_follow () { }
void op_forcenum () { }
void op_forchk1 () { }
void op_forinit () { }
void op_forintrrpt () { }
void op_forlcldob () { }
void op_forlcldol () { }
void op_forlcldow () { }
void op_forloop () { }
void op_gettruth () { }
void op_iretmvad () { }
void op_isformal () { }
void op_linefetch () { }
void op_linestart () { }
void op_mprofcallb () { }
void op_mprofcalll () { }
void op_mprofcallspb () { }
void op_mprofcallspl () { }
void op_mprofcallspw () { }
void op_mprofcallw () { }
void op_mprofexfun () { }
void op_mprofextcall () { }
void op_mprofextexfun () { }
void op_mprofforlcldob () { }
void op_mprofforlcldol () { }
void op_mprofforlcldow () { }
void op_mprofforloop () { }
void op_mproflinefetch () { }
void op_mproflinestart () { }
void op_namechk () { }
void op_neg () { }
void op_numcmp () { }
void op_pattern () { }
void op_restartpc () { }
void op_retarg () { }
void op_sorts_after () { }
void op_startintrrpt () { }
void op_sto () { }
void op_zbfetch () { }
void op_zbstart () { }
void op_zg1 () { }
void op_zgoto () { }
void op_zhelp () { }
void op_zst_fet_over () { }
void op_zst_st_over () { }
void op_zstepfetch () { }
void op_zstepstart () { }
void op_zstzb_fet_over () { }
void op_zstzb_st_over () { }
void op_zstzbfetch () { }
void op_zstzbstart () { }
void opp_break () { }
void opp_commarg () { }
void opp_fngvget1 () { }
void opp_hardret () { }
void opp_inddevparms () { }
void opp_indfnname () { }
void opp_indfun () { }
void opp_indget () { }
void opp_indglvn () { }
void opp_indlvadr () { }
void opp_indlvarg () { }
void opp_indlvnamadr () { }
void opp_indmerge () { }
void opp_indo2 () { }
void opp_indpat () { }
void opp_indrzshow () { }
void opp_indset () { }
void opp_indtext () { }
void opp_iretmval () { }
void opp_newintrinsic () { }
void opp_newvar () { }
void opp_ret () { }
void opp_rterror () { }
void opp_svput () { }
void opp_tcommit () { }
void opp_trestart () { }
void opp_trollback () { }
void opp_tstart () { }
void opp_xnew () { }
void opp_zcont () { }
void opp_zst_over_ret () { }
void opp_zst_over_retarg () { }
void opp_zstepret () { }
void opp_zstepretarg () { }
void output_symbol_size () { }
void patch_curr_blk () { }
void ping_keepalive () { }
void pseudo_ret () { }
void psock () { }
void put_tsiz () { }
void setupterm () { }
void shrink_trips () { }
void skip_white_space () { }
void tigetflag () { }
void tigetnum () { }
void tigetstr () { }
void tparm () { }
void tputs () { }
void var_start () { }
void zl_lab_err () { }
void zwr_output_buff () { }
