/****************************************************************
 *                                                              *
 *      Copyright 2001, 2003 Sanchez Computer Associates, Inc.  *
 *                                                              *
 *      This source code contains the intellectual property     *
 *      of its copyright holder(s), and is made available       *
 *      under a license.  If you do not know the terms of       *
 *      the license, please stop and do not read further.       *
 *                                                              *
 ****************************************************************/

#include "jnl_typedef.h"

/* New entries should be added at the end to maintain backward compatibility with previous journal files */
/* Note: This is an exception where we have 132+ characters in a line. It is needed so that from a
 *       particular number we can find record type. */

/* Please maintain jnl_rec_table.h parallel to this, in case this is changed */

/*
JNL_TABLE_ENTRY (rectype, extract_rtn,          label,    update,      fixed_size, is_replicated)
*/
JNL_TABLE_ENTRY (JRT_BAD,    NULL,             "*BAD*  ", NA,               FALSE, FALSE)  /* 0: Catch-all for invalid record types (must be first) */
JNL_TABLE_ENTRY (JRT_PINI,   mur_extract_pini, "PINI   ", NA,               TRUE,  FALSE)  /* 1: Process initialization */
JNL_TABLE_ENTRY (JRT_PFIN,   mur_extract_pfin, "PFIN   ", NA,               TRUE,  FALSE)  /* 2: Process termination */
JNL_TABLE_ENTRY (JRT_ZTCOM,  mur_extract_tcom, "ZTCOM  ", ZTCOMREC,         TRUE,  TRUE)   /* 3: End of "fenced" transaction */
JNL_TABLE_ENTRY (JRT_KILL,   mur_extract_set,  "KILL   ", KILLREC,          FALSE, TRUE)   /* 4: After-image logical journal transaction */
JNL_TABLE_ENTRY (JRT_FKILL,  mur_extract_set,  "FKILL  ", KILLREC|FUPDREC,  FALSE, TRUE)   /* 5: Like KILL, but the first in a "fenced" transaction */
JNL_TABLE_ENTRY (JRT_GKILL,  mur_extract_set,  "GKILL  ", KILLREC|GUPDREC,  FALSE, TRUE)   /* 6: Like FKILL, but not the first */
JNL_TABLE_ENTRY (JRT_SET,    mur_extract_set,  "SET    ", SETREC,           FALSE, TRUE)   /* 7: After-image logical journal transaction */
JNL_TABLE_ENTRY (JRT_FSET,   mur_extract_set,  "FSET   ", SETREC|FUPDREC,   FALSE, TRUE)   /* 8: Like SET, but the first in a "fenced" transaction */
JNL_TABLE_ENTRY (JRT_GSET,   mur_extract_set,  "GSET   ", SETREC|GUPDREC,   FALSE, TRUE)   /* 9: Like FSET, but not the first */
JNL_TABLE_ENTRY (JRT_PBLK,   mur_extract_blk,  "PBLK   ", NA,               FALSE, FALSE)  /* 10: Before-image physical journal transaction */
JNL_TABLE_ENTRY (JRT_EPOCH,  mur_extract_epoch,"EPOCH  ", NA,               TRUE,  FALSE)  /* 11: A "new epoch" */
JNL_TABLE_ENTRY (JRT_EOF,    mur_extract_eof,  "EOF    ", NA,               TRUE,  FALSE)  /* 12: End of file */
JNL_TABLE_ENTRY (JRT_TKILL,  mur_extract_set,  "TKILL  ", KILLREC|TUPDREC,  FALSE, TRUE)   /* 13: Like KILL, but the first in a TP transaction */
JNL_TABLE_ENTRY (JRT_UKILL,  mur_extract_set,  "UKILL  ", KILLREC|UUPDREC,  FALSE, TRUE)   /* 14: Like TKILL, but not the first */
JNL_TABLE_ENTRY (JRT_TSET,   mur_extract_set,  "TSET   ", SETREC|TUPDREC,   FALSE, TRUE)   /* 15: Like SET, but the first in a TP transaction */
JNL_TABLE_ENTRY (JRT_USET,   mur_extract_set,  "USET   ", SETREC|UUPDREC,   FALSE, TRUE)   /* 16: Like TSET, but not the first */
JNL_TABLE_ENTRY (JRT_TCOM,   mur_extract_tcom, "TCOM   ", TCOMREC,          TRUE,  TRUE)   /* 17: End of TP transaction */
JNL_TABLE_ENTRY (JRT_ALIGN,  mur_extract_align,"ALIGN  ", NA,               FALSE, FALSE)  /* 18: Align record */
JNL_TABLE_ENTRY (JRT_NULL,   mur_extract_null, "NULL   ", NA,               TRUE,  TRUE)   /* 19: Null record */
JNL_TABLE_ENTRY (JRT_ZKILL,  mur_extract_set,  "ZKILL  ", ZKILLREC,         FALSE, TRUE)   /* 20: After-image logical journal transaction */
JNL_TABLE_ENTRY (JRT_FZKILL, mur_extract_set,  "FZKILL ", ZKILLREC|FUPDREC, FALSE, TRUE)   /* 21: Like ZKILL, but the first in a "fenced" transaction */
JNL_TABLE_ENTRY (JRT_GZKILL, mur_extract_set,  "GZKILL ", ZKILLREC|GUPDREC, FALSE, TRUE)   /* 22: Like FZKILL, but not the first */
JNL_TABLE_ENTRY (JRT_TZKILL, mur_extract_set,  "TZKILL ", ZKILLREC|TUPDREC, FALSE, TRUE)   /* 23: Like ZKILL, but the first in a TP transaction */
JNL_TABLE_ENTRY (JRT_UZKILL, mur_extract_set,  "UZKILL ", ZKILLREC|UUPDREC, FALSE, TRUE)   /* 24: Like TZKILL, but not the first */
JNL_TABLE_ENTRY (JRT_INCTN,  mur_extract_inctn,"INCTN  ", NA,               TRUE,  FALSE)  /* 25: Increment curr_tn only, no logical update */
JNL_TABLE_ENTRY (JRT_AIMG,   mur_extract_blk,  "AIMG   ", NA,               FALSE, FALSE)  /* 26: After-image physical journal transaction */
