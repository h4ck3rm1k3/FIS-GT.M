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

/* mutex_deadlock_check -- mutex deadlock detection check

   There are 2 possible cases at this point:

   1) This is not a TP transaction
   2) This is a TP transaction

   For case 1, when we come in here we should not have crit in any other region (except as noted below)
   so instruct have_crit to complain about and release any such regions it finds.
   For case 2, we should not have crit in regions that are not part of this transaction and regions
   with an "ftok" that is higher than the region for which we are presently grabbing crit.
   Since tp_reg_list is sorted by ftok, we can just run this list in order and mark the regions that are allowed
   to have crit with our current cycle number.
*/

#include "mdef.h"

#include <netinet/in.h> /* Required for gtmsource.h */
#include <arpa/inet.h>

#ifdef VMS
#include <descrip.h> /* Required for gtmsource.h */
#endif

#include "gdsroot.h"
#include "gdsblk.h"
#include "gdskill.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "gdscc.h"
#include "filestruct.h"
#include "hashtab.h"
#include "buddy_list.h"
#include "jnl.h"
#include "tp.h"
#include "gtmimagename.h"
#include "repl_msg.h"		/* needed for gtmsource.h */
#include "gtmsource.h"		/* for jnlpool_addrs structure definition */

#include "have_crit.h"
#include "mutex_deadlock_check.h"

GBLREF	short		dollar_tlevel;
GBLREF	unsigned int	t_tries;
GBLREF	tp_region	*tp_reg_list;		/* Chained list of regions used in this transaction */
GBLREF	uint4		crit_deadlock_check_cycle;
GBLREF	boolean_t	is_replicator;
GBLREF	boolean_t	mu_reorg_process;
GBLREF	jnlpool_addrs	jnlpool;
GBLREF	gd_region	*gv_cur_region;
GBLREF	sgmnt_addrs	*cs_addrs;
GBLREF	boolean_t	in_mutex_deadlock_check;
GBLREF	volatile int4	crit_count;

void mutex_deadlock_check(mutex_struct_ptr_t criticalPtr)
{
	tp_region	*tr;
	sgmnt_addrs	*csa;
	int4		save_crit_count;

	if (in_mutex_deadlock_check)
		return;
	in_mutex_deadlock_check = TRUE;
	/* A zero value of "crit_count" implies asynchronous activities can occur (e.g. db flush timer, periodic epoch timers etc.).
	 * At this point, although we are here through grab_crit()/grab_lock() (which would have incremented "crit_count"), we are
	 * 	in a safe and consistent state as far as the mutex structures go so it is ok to set "crit_count" to 0 implying we
	 * 	are now in an interruptible state (of course, we need to restore "crit_count" to what it was before returning).
	 * The other alternative of not changing "crit_count" presents us with complex situations wherein recursion
	 * 	of grab_crit/rel_crit might occur (through direct or indirect calls from mutex_deadlock_check())
	 * 	causing crit_count to be > 1 and in turn causing the crit_count-reset-logic in grab_crit/rel_crit to
	 * 	do a "crit_count--" (instead of "crit_count = 0"). This suffers from the problem that in case of an error code path
	 * 	crit_count might not get decremented appropriately and hence become out-of-sync (i.e. a positive value instead
	 * 	of zero) and a non-zero value might cause indefinite deferrals of asynchronous events.
	 */
	assert(1 == crit_count);
	save_crit_count = crit_count;
	crit_count = 0;

	/* Need to determine who should and should not go through the deadlock checker.
	 *
	 * List of who needs to be considered
	 * ------------------------------------
	 * -> GT.M, Update process, MUPIP LOAD and GT.CM GNP/OMI server : since they go through t_end() to update the database.
	 * 	Note that all of the above (and only those) have the "is_replicator" flag set to TRUE.
	 * -> MUPIP REORG, since it does non-TP transactions and goes through t_end() (has "mu_reorg_process" flag set).
	 *
	 * List of who does not need to be considered (with reasons)
	 * -----------------------------------------------------------
	 * -> MUPIP RECOVER can hold crit on several regions (through TP or non-TP transactions).
	 * 	But it has standalone access and hence no possibility of a deadlock.
	 * -> MUPIP RESTORE too holds standalone access so does not need to be considered.
	 * -> Source Server, Receiver Server etc. can hold only one CRIT resource at any point of time.
	 * -> DSE, MUPIP BACKUP, MUPIP SET JOURNAL etc. can legitimately hold crit on several regions though in non-TP.
	 */
	if (is_replicator || mu_reorg_process)
	{
		if (0 == dollar_tlevel)
		{
			if ((NULL != jnlpool.jnlpool_dummy_reg) && jnlpool.jnlpool_dummy_reg->open)
			{
				++crit_deadlock_check_cycle;
				if (FILE_INFO(jnlpool.jnlpool_dummy_reg)->s_addrs.critical == criticalPtr)
				{	/* grab_lock going for crit on the jnlpool region. gv_cur_region points to the
					 * current region of interest, which better have replication enabled, and be now crit
					 */
					assert(cs_addrs == &FILE_INFO(gv_cur_region)->s_addrs);
					csa = &FILE_INFO(gv_cur_region)->s_addrs;
					if (FALSE == csa->now_crit || !REPL_ENABLED(csa->hdr))
						GTMASSERT;	/* should have crit on gv_cur_region before asking for jnlpool */
					csa->crit_check_cycle = crit_deadlock_check_cycle; /* allow for crit in gv_cur_region */
				}
			}
		} else
                {       /* Need to mark the regions allowed to have crit as follows:
                         * Place the current cycle into the csa's of regions allowed to have crit so have_crit() can easily test.
                         * Note that should the system be up long enough for the 2**32 cycle value to
                         * wrap and a region be unused for most of that time, such a region might not be entitled to crit
                         * but have an old csa->crit_cycle_check matching the current crit_deadlock_cycle_check -
                         * that case would not trigger have_crit() to release crit on that region;
                         * however, the next call to this routine increments crit_deadlock_check_cycle and so
                         * crit on that region gets released after two calls instead of (the usual) one.
			 */
			++crit_deadlock_check_cycle;
			for (tr = tp_reg_list;  NULL != tr;  tr = tr->fPtr)
			{
				if (!tr->reg->open)
					continue;
				csa = &FILE_INFO(tr->reg)->s_addrs;
				if (csa->now_crit)
					csa->crit_check_cycle = crit_deadlock_check_cycle;
				else
				{	/* Seen first non-crit region. Make sure either of the following is true.
					 *	 (i) this is the region we are currently grabbing crit on
					 *	(ii) we do not hold crit on any region in the tp_reg_list.
					 * If neither of the above, we have an out of design condition that can only
					 * 	warrant blowing the process up..
					 */
					if ((csa->critical != criticalPtr) && (tr != tp_reg_list))
						GTMASSERT;
					break;
				}
			}
		}
		/* Release crit in regions not legitimately part of this TP/non-TP transaction */
		have_crit(CRIT_RELEASE | CRIT_NOT_TRANS_REG);
	}
	crit_count = save_crit_count;
	in_mutex_deadlock_check = FALSE;
}
