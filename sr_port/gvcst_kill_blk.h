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

#ifndef GVCST_KILL_BLK_DEFINED

/* Declare parms for gvcst_kill_blk.c */

enum cdb_sc	gvcst_kill_blk (block_id blk,
				char level,
				sm_uc_ptr_t buffer,
				gv_key *search_key,
				srch_rec_status low,
				srch_rec_status high,
				bool right_extra,
				cw_set_element **cseptr);

#define GVCST_KILL_BLK_DEFINED

#endif
