use strict;
use warnings;

my $count =100;

foreach my $err (
    qw(

authenticate
cli_get_string_token
cli_gettoken
cli_is_assign
cli_is_dcm
cli_is_hex
cli_is_qualif
cli_lex_in_ptr
cli_lex_setup
cli_look_next_string_token
cli_look_next_token
cli_str_setup
cli_strupper
cli_token_buf
comp_linkages
conn_timeout
curr_entry
curr_entry follow
gtcm_ast_avail
gtcm_exi_condition
gtcm_gnp_server_log
gtcm_ltime
gtcm_stime
history
history follow
omi_brecv
omi_bsent
omi_conns
omi_debug
omi_errno
omi_exitp
omi_nerrs
omi_nxact
omi_nxact2
omi_pid
omi_pkdbg
omi_pklog
omi_pklog_addr
omi_service
omi_service follow
one_conn_per_inaddr
output_symbol_size
patch_curr_blk
patch_curr_blk follow
ping_keepalive
pow
psock
skip_white_space

))
{
    print "void $err () { }\n";
    $count++;
}
