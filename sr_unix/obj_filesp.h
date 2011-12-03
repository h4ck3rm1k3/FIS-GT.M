/****************************************************************
 *								*
 *	Copyright 2001, 2003 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#ifndef OBJ_FILESP_INCLUDED
#define OBJ_FILESP_INCLUDED

typedef struct linkage_entry
{
	struct linkage_entry	*next;
	struct sym_table	*symbol;
} linkage_entry;

struct sym_table *define_symbol(unsigned char psect, mstr *name);
void comp_linkages(void);
void resolve_sym (void);
void output_relocation (void);
void output_symbol (void);
void buff_emit(void);
void buff_flush(void);

/* Currently JSB contains two instructions:
 * 	load %ret, -1
 * 	ret
 #	nop	# (hppa)
 */
#ifdef __hpux
#  define JSB_ACTION_N_INS	3
#else
#  define JSB_ACTION_N_INS	2
#endif

#define JSB_MARKER		"GTM_CODE"
#define MIN_LINK_PSECT_SIZE	0
#define PADCHARS 		"PADDING PADDING"

#ifndef SECTION_ALIGN_BOUNDARY
#define SECTION_ALIGN_BOUNDARY	8
#endif

#endif
