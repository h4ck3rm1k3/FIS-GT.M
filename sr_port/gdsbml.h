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

#define SIXTEEN_BLKS_FREE	0x55555555
#define FOUR_BLKS_FREE		0x55
#define THREE_BLKS_FREE		0x54
#define LCL_MAP_LEVL		0xFF
#define BLK_BUSY		0x00
#define BLK_FREE		0x01
#define BLK_MAPINVALID		0x02
#define BLK_RECYCLED		0x03
#define BML_BITS_PER_BLK	2

#define	BM_MINUS_BLKHDR_SIZE(bplm)	((bplm) / (BITS_PER_UCHAR / BML_BITS_PER_BLK))
#define BM_SIZE(bplm)			(sizeof(blk_hdr) + BM_MINUS_BLKHDR_SIZE(bplm))

#define	VALIDATE_BM_BLK(blk, bp, csa, region, status)								\
{														\
	error_def(ERR_DBBMLCORRUPT);										\
														\
	assert(BITS_PER_UCHAR % BML_BITS_PER_BLK == 0);	/* assert this for the BM_MINUS_BLKHDR_SIZE macro */	\
	if (IS_BITMAP_BLK(blk) && ((LCL_MAP_LEVL != (bp)->levl) || (BM_SIZE(csa->hdr->bplmap) != (bp)->bsiz)))	\
	{													\
		send_msg(VARLSTCNT(9) ERR_DBBMLCORRUPT, 7, DB_LEN_STR(region), 					\
				blk, (bp)->bsiz, (bp)->levl, (bp)->tn, csa->ti->curr_tn);			\
		status = FALSE;											\
		assert(FALSE);											\
	} else													\
		status = TRUE;											\
}

#define NO_FREE_SPACE		-1
/* MAP_RD_FAIL is hard coded into the file BML_GET_FREE.MAR */
#define MAP_RD_FAIL		-2
#define EXTEND_SUSPECT		-3
#define FILE_EXTENDED		-4
#define	EXTEND_UNFREEZECRIT	-5

#define	GET_CDB_SC_CODE(gdsfilext_code, status)		\
{							\
	if (MAP_RD_FAIL == gdsfilext_code)		\
		status = rdfail_detail;			\
	else if (EXTEND_SUSPECT == gdsfilext_code)	\
		status = cdb_sc_extend;			\
	else if (NO_FREE_SPACE == gdsfilext_code)	\
		status = cdb_sc_gbloflow;		\
	else if (EXTEND_UNFREEZECRIT == gdsfilext_code)	\
		status = cdb_sc_unfreeze_getcrit;	\
}

#define MAXHARDCRITS		31

/* The following unfreeze-wait logic is lifted from similar stuff in tp_tend() */

#define	GRAB_UNFROZEN_CRIT(reg, csa, csd)				\
{									\
	int	lcnt;							\
	void	wcs_backoff();						\
									\
	assert(&FILE_INFO(reg)->s_addrs == csa && csa->hdr == csd);	\
	for (lcnt = 0; ; lcnt++)					\
	{								\
		grab_crit(reg);						\
		if (!csd->freeze)					\
			break;						\
		rel_crit(reg);						\
		if (MAXHARDCRITS < lcnt)				\
			wcs_backoff(lcnt);				\
	}								\
	assert(!csd->freeze && csa->now_crit);				\
}

#define MASTER_MAP_BITS_PER_LMAP	1

int4 bml_find_free(int4 hint, uchar_ptr_t base_addr, int4 total_bits, bool *used);
int4 bml_init(block_id bml);
uint4 bml_busy(uint4 setbusy, sm_uc_ptr_t map);
uint4 bml_free(uint4 setfree, sm_uc_ptr_t map);

