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

/* Common area to put all printing of stats on exit */
#include "mdef.h"

#include "gtmdbglvl.h"
#include "print_exit_stats.h"
#include "op.h"
#include "cache.h"

GBLREF	uint4		gtmDebugLevel;		/* Debug level (0 = using default sm module so with
						   a DEBUG build, even level 0 implies basic debugging) */

void print_exit_stats(void)
{
	if ((GDL_SmStats | GDL_SmDumpTrace | GDL_SmDump) & gtmDebugLevel)
		printMallocInfo();
#if !defined(__vax) && defined(DEBUG)
	if (GDL_PrintPieceStats & gtmDebugLevel)
		print_fnpc_stats();
#endif
	if (GDL_PrintIndCacheStats & gtmDebugLevel)
		cache_stats();
}
