/****************************************************************
 *								*
 *	Copyright 2001, 2004 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#ifndef GTM_ENV_XLATE_INIT_H
#define GTM_ENV_XLATE_INIT_H

void gtm_env_xlate_init(void);

GBLREF mstr		env_gtm_env_xlate;
//GBLREF int		(*gtm_env_xlate_entry)(); fgn_getrtn
GBLREF mval		dollar_zdir;

#define GTM_ENV_XLATE_ROUTINE_NAME "gtm_env_xlate"

error_def(ERR_XTRNTRANSERR);
error_def(ERR_XTRNTRANSDLL);
error_def(ERR_XTRNRETVAL);
error_def(ERR_XTRNRETSTR);
error_def(ERR_TEXT);

/* UNIX_ONLY (Any file that uses GTM_ENV_TRANSLATE macro should also include <limits.h>
 * which defines PATH_MAX macro) */

// declaration
//fgnfnc fgn_getrtn(void_ptr_t pak_handle, mstr *sym_name, int msgtype);

void DO_GTM_ENV_TRANSLATE(mval* VAL1, mval* VAL2) ;									


#define GTM_ENV_TRANSLATE(VAL1, VAL2) {	    DO_GTM_ENV_TRANSLATE(VAL1, VAL2) ; }

#endif	/*GTM_ENV_XLATE_INIT_H*/
