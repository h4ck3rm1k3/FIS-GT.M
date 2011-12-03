/****************************************************************
 *								*
 *	Copyright 2001, 2002 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#ifndef __RTNHDR_H__
#define __RTNHDR_H__

/* rtnhdr.h - routine header for shared binary Unix platforms */
#include "cache.h"

/* There are several references to this structure from assembly language; these include:

	From Unix:	g_msf.si

   Any changes to the routine header must be reflected in those files as well.

   Warning: the list above may not be complete.
*/

/* Variable table entry */
typedef mident var_tabent;

/* Linenumber table entry */
typedef int4 lnr_tabent;

/* Label table entry */
typedef struct
{
	mident		lab_name;		/* The name of the label */
	lnr_tabent	*lnr_adr;		/* Pointer to lnrtab entry offset into code for this label */
} lab_tabent;

/* Linkage table entry */
typedef struct
{
	char_ptr_t	ext_ref;		/* Address (quadword on alpha) this linkage entry resolves to or NULL */
} lnk_tabent;

/*	rhead_struct is the routine header; it occurs at the beginning of the
	object code part of each module. Since this structure may be resident in
	a shared library, this structure is considered inviolate. Therefore there is
	a process-private version of this header that is modified as necessary and
	always points to the current version.

	The routine header is initialized when a module is first linked into
	an executable. The fields marked with "(#)" are updated when the routine
	is replaced by a newer version via explicit zlink.
*/
typedef struct	rhead_struct
{
	char			jsb[RHEAD_JSB_SIZE];	/* GTM_CODE object marker */
	mstr			src_full_name;		/* (#) fully qualified path of routine source code */
	mident			routine_name;		/* external routine name */
	void_ptr_t		shlib_handle;		/* Null if header not for shared object. Non-zero means header is
							   describing shared library resident routine and this is its handle.
							   Note this is an 8 byte field on Tru64 (hence its position near top). */
	uint4			objlabel;		/* Object code level/label (see objlable.h) */
	var_tabent		*vartab_adr;		/* (#) address of variable table (offset in original rtnhdr) */
	int4			vartab_len;		/* (#) number of variable table entries */
	lab_tabent		*labtab_adr;		/* address of label table (offset in original rtnhdr) */
	int4			labtab_len;		/* number of label table entries */
	lnr_tabent		*lnrtab_adr;		/* address of linenumber table (offset in original rtnhdr) */
	int4			lnrtab_len;		/* number of linenumber table entries */
	unsigned char		*literal_text_adr;	/* address of literal text pool (offset in original rtnhdr) */
	int4			literal_text_len;	/* length of literal text pool */
	mval			*literal_adr;		/* (#) address of literal mvals (offset in original rtnhdr) */
	int4			literal_len;		/* number of literal mvals */
	lnk_tabent		*linkage_adr;		/* (#) address of linkage Psect (offset in original rtnhdr) */
	int4			linkage_len;		/* number of linkage entries */
	int4			rel_table_off;		/* offset to relocation table (not kept) */
	int4			sym_table_off;		/* offset to symbol table (not kept) */
	unsigned char		*shared_ptext_adr;	/* If set, ptext_adr points to local copy, this points to old shared copy */
	unsigned char		*ptext_adr;		/* (#) address of start of instructions (offset in original rtnhdr) */
	unsigned char		*ptext_end_adr;		/* (#) address of end of instructions + 1 (offset in original rtnhdr) */
	int4			checksum;		/* verification value */
	int4			temp_mvals;		/* (#) temp_mvals value of current module version */
	int4			temp_size;		/* (#) temp_size value of current module version */
	boolean_t		label_only;		/* was routine compiled for label only entry? */
	struct rhead_struct	*current_rhead_adr;	/* (#) address of routine header of current module version */
	struct rhead_struct	*old_rhead_adr;		/* (#) chain of replaced routine headers */
} rhdtyp;

/* Routine table entry */
typedef struct
{
	mident	rt_name;
	rhdtyp	*rt_adr;
} rtn_tabent;

/* Macros for accessing routine header fields in a portable way */
#define VARTAB_ADR(rtnhdr) ((rtnhdr)->vartab_adr)
#define LABTAB_ADR(rtnhdr) ((rtnhdr)->labtab_adr)
#define LNRTAB_ADR(rtnhdr) ((rtnhdr)->lnrtab_adr)
#define LITERAL_ADR(rtnhdr) ((rtnhdr)->literal_adr)
#define LINKAGE_ADR(rtnhdr) ((rtnhdr)->linkage_adr)
#define PTEXT_ADR(rtnhdr) ((rtnhdr)->ptext_adr)
#define PTEXT_END_ADR(rtnhdr) ((rtnhdr)->ptext_end_adr)
#define CURRENT_RHEAD_ADR(rtnhdr) ((rtnhdr)->current_rhead_adr)
#define OLD_RHEAD_ADR(rtnhdr) ((rtnhdr)->old_rhead_adr)
#define LINE_NUMBER_ADDR(rtnhdr, lnr_tabent_adr) ((rtnhdr)->ptext_adr + *(lnr_tabent_adr))
#define LABENT_LNR_ENTRY(rtnhdr, lab_tabent_adr) ((lab_tabent_adr)->lnr_adr)
#define LABEL_ADDR(rtnhdr, lab_tabent_adr) (CODE_BASE_ADDR(rtnhdr) + *(LABENT_LNR_ENTRY(rtnhdr, lab_tabent_adr)))
#define CODE_BASE_ADDR(rtnhdr) ((rtnhdr)->ptext_adr)
#define CODE_OFFSET(rtnhdr, addr) ((char *)(addr) - (char *)(rtnhdr->ptext_adr))

/* Macro to determine if given address is inside code segment. Note that even though
   the PTEXT_END_ADR macro is the address of end_of_code + 1, we still want a <= check
   here because in many cases, the address being tested is the RETURN address from a
   call that was done as the last instruction in the code segment. Sometimes this call
   is to an error or it could be the implicit quit. On HPUX, the delay slot for the
   implicit quit call at the end of the module can also cause the problem. Without
   the "=" check also being there, the test will fail when it should succeed.
*/
#define ADDR_IN_CODE(caddr, rtnhdr) (PTEXT_ADR((rtnhdr)) <= (caddr) && (caddr) <= PTEXT_END_ADR((rtnhdr)))

/* Types that are different depending on shared/unshared unix binaries */
#define LAB_TABENT lab_tabent
#define LNR_TABENT lnr_tabent
#define RTN_TABENT rtn_tabent
#define VAR_TABENT var_tabent
#define LABENT_LNR_OFFSET lnr_adr
#define RTNENT_RT_ADR rt_adr

/* Following is the indirect routine header build as part of an indirect code object */
typedef struct ihead_struct
{
	cache_entry	*indce;
	int4		vartab_off;
	int4		vartab_len;
	int4		temp_mvals;
	int4		temp_size;
	int4		fixup_vals_off;
	int4		fixup_vals_num;
} ihdtyp;

/*
 * Format of a relocation datum.
 */
struct	relocation_info
{
		 int	r_address;	/* address which is relocated */
	unsigned int	r_symbolnum;	/* local symbol ordinal */
};

struct	rel_table
{
	struct rel_table	*next,
				*resolve;
	struct relocation_info	r;
};

/*
 * Format of a symbol table entry; this file is included by <a.out.h>
 * and should be used if you aren't interested the a.out header
 * or relocation information.
 */
struct	nlist
{
	int4		n_type;		/* type flag, i.e. N_TEXT etc; see below */
	uint4		n_value;	/* value of this symbol (or sdb offset) */
};

struct	sym_table
{
	struct sym_table	*next;
	struct nlist		n;
	struct rel_table	*resolve;
	int4			linkage_offset;
	unsigned short		name_len;
	unsigned char		name[1];
};

/*
 * Simple values for n_type.
 */
#define	N_TEXT	0x04		/* text */
#define	N_EXT	0x01		/* external bit, or'ed in */

/* Prototypes */
void indir_lits(ihdtyp *ihead);
int get_src_line(mval *routine, mval *label, int offset, mstr **srcret);
unsigned char *find_line_start(unsigned char *in_addr, rhdtyp *routine);
int4 *find_line_addr(rhdtyp *routine, mstr *label, short int offset);
rhdtyp *find_rtn_hdr(mstr *name);
bool zlput_rname(rhdtyp *hdr);
rhdtyp *make_dmode(void);
void comp_lits(rhdtyp *rhead);
rhdtyp  *op_rhdaddr(mval *name, rhdtyp *rhd);
lnr_tabent **op_labaddr(rhdtyp *routine, mval *label, int4 offset);
void urx_resolve(rhdtyp *rtn, lab_tabent *lbl_tab, lab_tabent *lbl_top);
char *rtnlaboff2entryref(char *entryref_buff, mstr *rtn, mstr *lab, int offset);

#endif
