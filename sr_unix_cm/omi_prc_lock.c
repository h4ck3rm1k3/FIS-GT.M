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

/*
 *  omi_prc_lock.c ---
 *
 *	Process an LOCK request.
 *
 */

#ifndef lint
static char rcsid[] = "$Header: /cvsroot/fis-gtm/gtm/sr_unix_cm/omi_prc_lock.c,v 1.1.1.1 2001/05/16 14:01:54 marcinim Exp $";
#endif

#include "mdef.h"
#include "omi.h"
#include "error.h"
#include "mlkdef.h"
#include "mlk_lock.h"
#include "mlk_pvtblk_delete.h"
#include "op.h"


int
omi_prc_lock(omi_conn *cptr, char *xend, char *buff, char *bend)
{
    GBLREF int		 omi_pid;
    GBLREF mlk_pvtblk	*mlk_pvt_root;

    char		*bptr;
    omi_li		 li;
    int			 rv;
    omi_si		 si;
    mlk_pvtblk		*next, **prior;

    bptr = buff;

/*  Global Ref */
    OMI_LI_READ(&li, cptr->xptr);
/*  Condition handler for DBMS operations */
    ESTABLISH(omi_dbms_ch);

/*  Clean up dead locks in private list */
    for (prior = &mlk_pvt_root; *prior; )
    {	if (!(*prior)->granted || !(*prior)->nodptr || (*prior)->nodptr->owner != omi_pid)
		mlk_pvtblk_delete(prior);
	else
	{	(*prior)->trans = 0;
		prior = &(*prior)->next;
	}
    }

    rv = omi_lkextnam(cptr, li.value, cptr->xptr, cptr->xptr + li.value);
/*  If true, there was an error locking the global reference in the DBMS */
    if (rv < 0) {
	REVERT;
	return rv;
    }
    cptr->xptr += li.value;

/*  Client's $JOB */
    OMI_SI_READ(&si, cptr->xptr);
    cptr->xptr += si.value;

/*  Bounds checking */
    if (cptr->xptr > xend) {
	REVERT;
	return -OMI_ER_PR_INVMSGFMT;
    }

/*  If true, we did not get the lock on the name */
    if (rv == 0) {
	OMI_SI_WRIT(0, bptr);
	REVERT;
	return bptr - buff;
    }

    mlk_pvt_root->trans = 0;
    if (!(rv = mlk_lock(mlk_pvt_root, (uint4)cptr, FALSE)))
    {	    /* Lock succeeded; inform the client */
	mlk_pvt_root->granted = TRUE;
	mlk_pvt_root->level++;
    }
    rv = !rv;

    if (!mlk_pvt_root->nodptr || mlk_pvt_root->nodptr->owner != omi_pid)
    {	next = mlk_pvt_root->next;
	gtm_free_intern(mlk_pvt_root);
	mlk_pvt_root = next;
	rv = 0;
    }

    OMI_SI_WRIT(rv, bptr);
    REVERT;
    return bptr - buff;
}
