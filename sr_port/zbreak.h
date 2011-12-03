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

#ifndef ZBREAK_H_INCLUDED
#define ZBREAK_H_INCLUDED

#include "zbreaksp.h"
#include "cache.h"

typedef struct
{
	zb_code		*mpc;		/* MUMPS address for ZBREAK */
	mident		rtn;
	mident		lab;
	int		offset;
	int		count;		/* # of time ZBREAK encountered */
	cache_entry	*action;	/* action associated with ZBREAK (indirect cache entry) */
	zb_code 	m_opcode;	/* MUMPS op_code replaced */
} zbrk_struct;

typedef struct
{
	zbrk_struct	*beg;
	zbrk_struct	*free;
	zbrk_struct	*end;
} z_records;

#ifndef ZB_CODE_SHIFT
#define ZB_CODE_SHIFT 0
#endif

#define INIT_NUM_ZBREAKS 1
#define CANCEL_ONE -1
#define CANCEL_ALL -2

#ifdef __MVS__		/* need to adjust for load address inst. (temporary) */
#define SIZEOF_LA	4
#else
#define SIZEOF_LA	0
#endif

zbrk_struct *zr_find(z_records *zrecs, zb_code *addr);
zbrk_struct *zr_get_free(z_records *zrecs, zb_code *addr);
void zr_init(z_records *zrecs, int4 count);
void zr_put_free(z_records *zrecs, zbrk_struct *z_ptr);
zb_code *find_line_call(void *addr);
void zr_remove(rhdtyp *rtn);

#endif /* ZBREAK_H_INCLUDED */
