/****************************************************************
 *								*
 *	Copyright 2001 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#ifndef MDESP_included
#define MDESP_included

#include <sys/types.h>

typedef          long	int4;		/* 4-byte signed integer */
typedef unsigned long	uint4;		/* 4-byte unsigned integer */

#define INT8_SUPPORTED
#define	INT8_FMT		"%llu"
#define	INT8_FMTX		"[0x%llx]"

/* Starting off life as debugging parms and now we need them for the
   short term so define them here */
#define DEBUG_LEAVE_SM
#define DEBUG_NOMSYNC

#define readonly
#define GBLDEF
#define GBLREF extern
#define LITDEF
#define LITREF extern
#define error_def(x) LITREF int x
#ifdef DEBUG
error_def(ERR_ASSERT);
#define assert(x) ((x) ? 1 : rts_error(VARLSTCNT(5) ERR_ASSERT, 3, sizeof(__FILE__) - 1, __FILE__, __LINE__))
#else
#define assert(x)
#endif

#define UNIX 1
#undef VMS
#define PADDING 1
#define BIGENDIAN 1
#define CNTR_WORD_32

/* #define memcmp(DST,SRC,LEN) memucmp(DST,SRC,LEN) */

#ifdef __sparc
#define CACHELINE_SIZE        256
#define MSYNC_ADDR_INCS        OS_PAGE_SIZE

#define OFF_T_LONG
#define INO_T_LONG
#undef sssize_t
#define sssize_t ssize_t
#undef SHMDT
#define SHMDT(X) shmdt((char *)(X))

/* Use rc_mval2subsc only for sun until every DTM client (that needs 16-bit precision as opposed to 18-bit for GT.M) is gone */
#define	mval2subsc	rc_mval2subsc
#endif

#ifdef __hpux
#define CACHELINE_SIZE        64
#define MSYNC_ADDR_INCS        OS_PAGE_SIZE

#define OFF_T_LONG
#endif

#ifdef __linux__
#define OFF_T_LONG
typedef unsigned short	in_port_t;
#endif

#ifdef __i386
/* Through Pentium Pro/II/III, should use CPUID to get real value perhaps */
#define CACHELINE_SIZE        32
#define MSYNC_ADDR_INCS        OS_PAGE_SIZE
#undef BIGENDIAN
#endif

#define INTERLOCK_ADD(X,Y,Z)    (add_inter(Z, (sm_int_ptr_t)(X), (sm_global_latch_ptr_t)(Y)))


/* Reserve enough space in routine header for the dummy string "GTM_CODE".  */
#define RHEAD_JSB_SIZE	8

typedef struct
{
#ifdef	BIGENDIAN
	unsigned int	sgn : 1 ;
	unsigned int	e   : 7 ;
#else
	unsigned int	e   : 7 ;
	unsigned int	sgn : 1 ;
#endif
	int4		m[2]	;
} mflt ;

typedef struct
{
	unsigned int	mvtype   : 8;
#ifdef	BIGENDIAN
	unsigned int	sgn      : 1;
	unsigned int	e        : 7;
#else
	unsigned int	e        : 7;
	unsigned int	sgn      : 1;
#endif
	unsigned int	fnpc_indx: 16;  /* Index to fnpc_work area this mval is using */
	mstr	str;
	int4	m[2];
} mval ;


#ifdef BIGENDIAN
#define DEFINE_MVAL_LITERAL(TYPE, EXPONENT, SIGN, LENGTH, ADDRESS, MANT_LOW, MANT_HIGH) \
	{TYPE, SIGN, EXPONENT, 0xffff, LENGTH, ADDRESS, MANT_LOW, MANT_HIGH}
#else
#define DEFINE_MVAL_LITERAL(TYPE, EXPONENT, SIGN, LENGTH, ADDRESS, MANT_LOW, MANT_HIGH) \
	{TYPE, EXPONENT, SIGN, 0xffff, LENGTH, ADDRESS, MANT_LOW, MANT_HIGH}
#endif

//#define VAR_STARTo(a)	va_start(a,foo)
#define VARLSTCNT(a)	a,		/* push count of arguments*/

#ifndef GTSQL    /* these cannot be used within SQL */
#define malloc gtm_malloc
#define free gtm_free
#endif

#define CODE_ADDRESS(func)	(unsigned char *)func
#define	CONTEXT(func)		0	/* not used on this target */

/* PSECT in which the address of the module is defined: */
#define GTM_MODULE_DEF_PSECT	GTM_CODE


#define OS_PAGELET_SIZE		512
#define OS_VIRTUAL_BLOCK_SIZE	OS_PAGELET_SIZE
#define GTM_MM_FLAGS		MAP_SHARED
#define SSM_SIZE                OS_PAGE_SIZE

typedef volatile	int4    latch_t;
typedef volatile	uint4   ulatch_t;

#define INSIDE_CH_SET		"ISO8859-1"
#define OUTSIDE_CH_SET		"ISO8859-1"
#define EBCDIC_SP		0x40
#define NATIVE_SP		0x20
#define DEFAULT_CODE_SET	ascii	/* enum ascii defined in io.h */

#endif /* MDESP_included */
