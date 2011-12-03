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

/*
 *  gtcm_rep_err.c ---
 *
 *	Error logging facility.
 *
 */

#ifndef lint
static char rcsid[] = "$Header: /cvsroot/fis-gtm/gtm/sr_unix_cm/gtcm_rep_err.c,v 1.2 2002/06/13 14:34:39 miazim Exp $";
#endif

#include "mdef.h"

#include "gtm_string.h"
#include "gtm_stdio.h"
#include "gtm_stdlib.h"
#include "gtm_syslog.h"
#include "gtm_time.h"

#include "gtcm.h"
#include "fao_parm.h"
#include "eintr_wrappers.h"

GBLREF char 		util_outbuff[], *util_outptr;
GBLREF char		*omi_service;
static boolean_t 	first_syslog = TRUE;

void gtcm_rep_err(char *msg, int errcode)
{
    FILE	*fp;
    char	outbuf[2048];
    mstr	msgstr;
    unsigned int faol[MAX_FAO_PARMS];
    void	gtm_getmsg(), util_out_print();
    time_t	now;
    int		status;
    char *gtm_dist, fileName[256];

    msgstr.addr = outbuf;
    msgstr.len = sizeof(outbuf);
    gtm_getmsg(errcode, &msgstr);
    memset(&faol[0], 0, sizeof(faol));

    util_out_print(0, 2, 0);	/* reset error buffer */;
    util_out_print(msgstr.addr,FALSE,faol[0],faol[1],faol[2],faol[3],faol[4],
	faol[5],faol[6],faol[7],faol[8],faol[9],faol[10],faol[11]);
    util_out_print("!/",FALSE);
    *util_outptr++ = 0;

    if (gtm_dist = GETENV("gtm_dist"))
	    SPRINTF(fileName,"%s/log/gtcm_server.erlg", gtm_dist);
    else
	    strcpy(fileName,"/usr/tmp/gtcm_server.erlg");

    if ((fp = Fopen(fileName, "a")))
    {
		now=time(0);
		FPRINTF(fp, "%s", GTM_CTIME(&now));
		FPRINTF(fp,"server(%s)  %s:  %s", omi_service, msg, util_outbuff);
		FCLOSE(fp, status);
    }

#ifdef BSD_LOG
    if (first_syslog)
    {
		first_syslog = FALSE;
		(void)OPENLOG("GTCM", LOG_PID | LOG_CONS | LOG_NOWAIT, LOG_USER);
	}
    SYSLOG(LOG_ERR, util_outbuff);
#endif /* defined(BSD_LOG) */

    return;
}
