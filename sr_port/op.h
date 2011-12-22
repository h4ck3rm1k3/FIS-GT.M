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

#ifndef OP_INCLUDED
#define OP_INCLUDED




boolean_t op_gvqueryget(mval *key, mval *val);
int op_dt_get(void);
void op_fnzdate(mval *src, mval *fmt, mval *mo_str, mval *day_str, mval *dst);
int op_fnzsearch(mval *file, mint indx, mval *ret);		/***type int added***/
bool op_gvget(mval *v);
int op_incrlock(int timeout);
int op_lock(int timeout);
int op_open(mval *device, mval *devparms, int timeout, mval *mspace);
int op_rdone(mval *v, int4 timeout);
int op_readfl(mval *v, int4 length, int4 timeout);
int op_read(mval *v, int4 timeout);
int op_zallocate(int timeout);		/***type int added***/
int op_fngvget1(mval *v);
int op_zalloc2(int4 timeout, uint4 auxown);
int op_zallocate(int timeout);
void op_unwind(void);
void op_break(void);
void op_lvpatwrite(int,...)/*AUTO*/;
void op_killall(void);
void op_gvzwithdraw(void);
void op_gvkill(void);
void op_cat(int,...);
void op_close(mval *v, mval *p);
void op_commarg(mval *v, unsigned char argcode);
void op_decrlock(int4 timeout);
void op_dmoe(void);
void op_div(mval *u, mval *v, mval *q);
void op_exp(mval *u, mval* v, mval *p);
int4 op_fnfind(mval *src, mval *del, mint first, mval *dst);
void op_fnfnumber(mval *src,mval *fmt,mval *dst);
void op_fnj2(mval *src,int len,mval *dst);
void op_fnj3(mval *src,int width,int fract,mval *dst);
void op_fnlvname(mval *src, mval *dst);
void op_fnlvnameo2(mval *src,mval *dst,mval *direct);
void op_fnfgncal (uint4 n_mvals, mval *dst, mval *package, mval *extref, uint4 mask, int4 argcnt, ...);

#ifdef __sun
int op_fnfgncal_rpc(int,...)/*AUTO*/;	/* typ to keep the compiler happy as set into xfer_table, which is int */
#endif
void op_fngvget(mval *v, mval *def);
void op_fngetjpi(mint jpid, mval *kwd, mval *ret);
void op_fnlvprvname(mval *src, mval *dst);
void op_fnname(int,...)/*AUTO*/;
void op_fnqlength(mval *name, mval *subscripts);
void op_fnqsubscript(mval *name, int seq, mval *subscript);
void op_fnrandom(int4 interval, mval *ret);
void op_fnreverse(mval *src, mval *dst);
void op_fnstack1(int level, mval *result);
void op_fnstack2(int level, mval *info, mval *result);
void op_fnview(int,...);
void op_fnpiece(mval *src, mval *del, int first, int last, mval *dst, boolean_t srcisliteral);
void op_fnquery(int,...)/*AUTO*/;
void op_fntext(mval *label, int int_exp, mval *rtn, mval *ret);
void op_fnzbitand(mval *dst, mval *bitstr1, mval *bitstr2);
void op_fnzbitcoun(mval *dst, mval *bitstr);
void op_fnzbitget(mval *dst, mval *bitstr, int pos);
void op_fnzbitlen(mval *dst, mval *bitstr);
void op_fnzbitor(mval *dst, mval *bitstr1, mval *bitstr2);
void op_fnzbitstr(mval *bitstr, int size, int truthval);
void op_fnzjobexam(mval *prelimSpec, mval *finalSpec);
void op_fnzsigproc(int processid, int signum, mval *retcode);
void op_fnzlkid(mint boolex, mval *retval);
void op_fnzqgblmod(mval *v);

void op_fnzcall(int,...)/*AUTO*/;
void op_fnzpid(mint boolexpr,mval *ret);
void op_fnzpriv(mval *prv,mval *ret);
void op_fngetsyi(mval *keyword,mval *node,mval *ret);
void op_gvdata(mval *v);
void op_gvextnam(int, ...);
void op_gvnaked(int,...);
void op_gvname(int,...);
void op_gvnext(mval *v);
void op_gvorder(mval *v);
void op_gvo2(mval *dst,mval *direct);
void op_gvput(mval *var);
void op_gvquery(mval *v);
void op_gvrectarg(mval *v);
void op_halt(void);
void op_hang(mval *num);
void op_hardret(void);
void op_horolog(mval *s);
int op_incrlock(int timeout);
void op_iocontrol(int, ...);
void op_indrzshow(mval *s1,mval *s2);
void op_iretmval(mval *v);
int op_job(int,...)/*AUTO*/;
void op_killall(void);
void op_lkinit(void);
void op_lkname(int,...)/*AUTO*/;
void op_mul(mval *u, mval *v, mval *p);
void op_newvar(uint4 arg1);
void op_newintrinsic(int intrtype);
void op_oldvar(void);
void op_population(mval *arg1, mval *arg2, mval *dst);
void op_rterror(int4 sig, boolean_t subrtn);
void op_setp1(mval *src, int delim, mval *expr, int ind, mval *dst);
void op_setpiece(mval *src, mval *del, mval *expr, int4 first, int4 last, mval *dst);
void op_fnzsetprv(mval *prv,mval *ret);
void op_fnztrnlnm(mval *name,mval *table,int4 ind,mval *mode,mval *case_blind,mval *item,mval *ret);

void op_setzbrk(mval *rtn, mval *lab, int offset, mval *act, int cnt);
void op_sqlinddo(mstr *m_init_rtn);

void op_svget(int varnum, mval *v);
void op_svput(int varnum, mval *v);
void op_tcommit(void);
void op_trollback(int rb_levels);
void op_tstart(int,...);//TODO
void op_unlock(void);
void op_use(mval *v, mval *p);
void op_view(int,...);
void op_write(mval *v);
void op_wteol(int4 n);
void op_wtff(void);
void op_wtone(unsigned char c);
void op_wttab(mint x);
void op_xkill(int,...)/*AUTO*/;
void op_xnew(int,...);
void op_zattach(mval *);
void op_zcompile(mval *v);
void op_zcont(void);
void op_zdealloc2(int4 timeout, uint4 auxown);
void op_zdeallocate(int4 timeout);
void op_zedit(mval *v, mval *p);
void op_zhelp_xfr(mval *subject, mval *lib);
void op_zlink(mval *v, mval *quals);
void op_zmess(int,...)/*AUTO*/;
void op_zprevious(mval *v);
void op_zprint(mval *rtn,mval *start_label,int start_int_exp,mval *end_label,int end_int_exp);
void op_zst_break(void);
void op_zstep(uint4 code, mval *action);
void op_zsystem(mval *v);
void op_ztcommit(int4 n);
void op_ztstart(void);
#ifdef UNIX
int op_fetchintrrpt(int,...)/*AUTO*/, op_startintrrpt(int,...)/*AUTO*/, op_forintrrpt(int,...)/*AUTO*/;
#elif defined(VMS)
void op_fetchintrrpt(int,...)/*AUTO*/, op_startintrrpt(int,...)/*AUTO*/, op_forintrrpt(int,...)/*AUTO*/;
#else
#error unsupported platform
#endif
int op_forchk1(int,...)/*AUTO*/, op_forloop(int,...)/*AUTO*/;
int op_zstepfetch(int,...)/*AUTO*/, op_zstepstart(int,...)/*AUTO*/, op_zstzbfetch(int,...)/*AUTO*/, op_zstzbstart(int,...)/*AUTO*/;
int op_mproflinestart(int,...)/*AUTO*/, op_mproflinefetch(int,...)/*AUTO*/, op_mprofforloop(int,...)/*AUTO*/;
int op_linefetch(int,...)/*AUTO*/, op_linestart(int,...)/*AUTO*/, op_zbfetch(int,...)/*AUTO*/, op_zbstart(int,...)/*AUTO*/, op_ret(int,...)/*AUTO*/, op_retarg(int,...)/*AUTO*/;
int opp_ret(int,...)/*AUTO*/;
int op_zst_fet_over(int,...)/*AUTO*/, op_zst_st_over(int,...)/*AUTO*/, op_zstzb_st_over(int,...)/*AUTO*/, opp_zstepret(int,...)/*AUTO*/, opp_zstepretarg(int,...)/*AUTO*/;
int op_zstzb_fet_over(int,...)/*AUTO*/, opp_zst_over_ret(int,...)/*AUTO*/, opp_zst_over_retarg(int,...)/*AUTO*/;
#ifndef __MVS__
void fetch(int,...);
#else
void gtm_fetch(int,...)/*AUTO*/;
#endif
void add_mvals(mval *u, mval *v, int subtraction, mval *result);
void op_bindparm(int,...);
void op_add(mval *u, mval *v, mval *s);
void op_sub(mval *u, mval *v, mval *s);

void op_cvtparm(int iocode, mval *src, mval *dst);
void op_dmode(void);
void op_dt_false(void);
void op_dt_store(int truth_value);
void op_dt_true(void);
void op_fnascii(int4 num, mval *in, mval *out);
void op_fnchar(int,...);
void op_fnget2(mval *dst, mval *src, mval *defval);
void op_fngetdvi(mval *device, mval *keyword, mval *ret);
void op_fngetlki(mval *lkid_mval, mval *keyword, mval *ret);
int op_fngvget2(mval *res, mval *val, mval *optional);
void op_fnp1(mval *src, int del, int trgpcidx, mval *dst, boolean_t srcisliteral);
#ifdef DEBUG
void print_fnpc_stats(void);
#endif
void op_fntranslate(mval *src,mval *in_str,mval *out_str,mval *dst);
void op_fnzbitfind(mval *dst, mval *bitstr, int truthval, int pos);
void op_fnzbitnot(mval *dst,mval *bitstr);
void op_fnzbitset(mval *dst, mval *bitstr, int pos, int truthval);
void op_fnzbitxor(mval *dst, mval *bitstr1, mval *bitstr2);
void op_fnzfile(mval *name,mval *key,mval *ret);
void op_fnzm(mint x,mval *v);
void op_fnzparse(mval *file, mval *field, mval *def1, mval *def2, mval *type, mval *ret);
void op_fnzsqlexpr(mval *value, mval *target);
void op_fnzsqlfield(int findex, mval *target);
void op_gvsavtarg(mval *v);
void op_gvzwrite(int,...); //todo ansi
void op_idiv(mval *u, mval *v, mval *q);
void op_igetsrc(mval *v);
int op_lock2(int4 timeout, unsigned char laflag);
void op_inddevparms(mval *devpsrc, int4 ok_iop_parms, mval *devpiopl);
void op_indfnname(mval *dst, mval *target, int value);
void op_indfun(mval *v, mint argcode, mval *dst);
void op_indget(mval *dst, mval *target, mval *value);
void op_indglvn(mval *v,mval *dst);
void op_indlvadr(mval *target);
void op_indlvarg(mval *v,mval *dst);
void op_indlvnamadr(mval *target);
void op_indmerge(mval *glvn_mv, mval *arg1_or_arg2);
void op_indname(int,...)/*AUTO*/;
void op_indpat(mval *v, mval *dst);
void op_indo2(mval *dst, mval *target, mval *value);
void op_indset(mval *target, mval *value);
void op_indtext(mval *lab, mint offset, mval *rtn, mval *dst);
void op_lvzwrite(int,...); // todo ansi
void op_nullexp(mval *v);
int op_open_dummy(mval *v, mval *p, int t, mval *mspace);
void op_setextract(mval *src, mval *expr, int schar, int echar, mval *dst);
void op_trestart(int newlevel);
void op_zst_over(void);
void op_zstepret(void);

#endif
