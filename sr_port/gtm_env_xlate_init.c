/****************************************************************
 *                                                              *
 *      Copyright 2001, 2002 Sanchez Computer Associates, Inc.  *
 *                                                              *
 *      This source code contains the intellectual property     *
 *      of its copyright holder(s), and is made available       *
 *      under a license.  If you do not know the terms of       *
 *      the license, please stop and do not read further.       *
 *                                                              *
 ****************************************************************/

#include "mdef.h"

#include "gtm_string.h"
#include "gtm_limits.h"

#include "iosp.h"
#include "trans_log_name.h"
#include "gtm_logicals.h"
#include "error.h"
#include "gtm_env_xlate_init.h"
#include "stringpool.h"
#include "../sr_unix/fgncalsp.h"


void gtm_env_xlate_init(void)
{
	uint4		status;
	mstr		val, tn;
	char		buf[PATH_MAX];

	error_def(ERR_TRNLOGFAIL);

	val.addr = ZGTMENVXLATE;
	val.len =  STR_LIT_LEN(ZGTMENVXLATE);
	if (SS_NORMAL == (status = trans_log_name(&val, &tn, buf)))
	{
		UNIX_ONLY(
			env_gtm_env_xlate.len = tn.len;
			env_gtm_env_xlate.addr = (char *)gtm_malloc_intern(tn.len);
			memcpy(env_gtm_env_xlate.addr, buf, tn.len);
			)
		VMS_ONLY(
			/* In op_gvextnam, the logical name is used in VMS,
			 * rather than its value (by lib$find_image_symbol),
			 * so only whether the logical name translates is
			 * checked here.
			 */
			env_gtm_env_xlate.len = val.len;
			env_gtm_env_xlate.addr = val.addr;
			)
	}
	else if  (SS_NOLOGNAM == status)
		env_gtm_env_xlate.len = 0;
	else
		rts_error(VARLSTCNT(5) ERR_TRNLOGFAIL, 2, LEN_AND_LIT(ZGTMENVXLATE), status);

	return;
}



//void_ptr_t	fgn_getpak(const char *, int ERROR){}

void DO_GTM_ENV_TRANSLATE(mval* VAL1, mval* VAL2) {									
  mval val_xlated;

	if (0 != env_gtm_env_xlate.len)									
	{		

	  //#define MSTR_CONST(name,string) mstr name = { LEN_AND_LIT(string) }
	  //#define MSTR_DEF(name,length,string) mstr name = { length, string }
										
		MSTR_CONST(routine_name, GTM_ENV_XLATE_ROUTINE_NAME);					
		int             ret_gtm_env_xlate;							
		UNIX_ONLY(										
			char		pakname[PATH_MAX + 1];						
			void_ptr_t	pakhandle;							
		)										       
		VMS_ONLY(										
			int4                    status;							
			struct dsc$descriptor   filename;						
			struct dsc$descriptor   entry_point;						
		)											
		MV_FORCE_STR(VAL2);									


		///
		memcpy(pakname, env_gtm_env_xlate.addr, env_gtm_env_xlate.len);			
		pakname[env_gtm_env_xlate.len]='\0';						
		pakhandle = fgn_getpak(pakname, ERROR);						



		val_xlated.str.addr = NULL;							


		fgnfnc gtm_env_xlate_entry = fgn_getrtn(pakhandle, &routine_name, ERROR);	

		//fgnfnc fgn_getrtn(void_ptr_t pak_handle, mstr *sym_name, int msgtype);
		
		ret_gtm_env_xlate = gtm_env_xlate_entry(
							&(VAL1)->str, 
							&(VAL2)->str, 
							&(dollar_zdir.str)
					       );	 //(int)&(val_xlated.str)
		if (MAX_DBSTRLEN < val_xlated.str.len)							
			rts_error(VARLSTCNT(4) ERR_XTRNRETVAL, 2, val_xlated.str.len, MAX_DBSTRLEN);				
		if (0 != ret_gtm_env_xlate)									
		{												
			if ((val_xlated.str.len) && (val_xlated.str.addr))					
				rts_error(VARLSTCNT(6) ERR_XTRNTRANSERR, 0, ERR_TEXT,  2, val_xlated.str.len, 	
					val_xlated.str.addr);							
			else											
				rts_error(VARLSTCNT(1) ERR_XTRNTRANSERR);					
		}												
		if ((NULL == val_xlated.str.addr) && (0 != val_xlated.str.len))					
			rts_error(VARLSTCNT(1)ERR_XTRNRETSTR);							
		val_xlated.mvtype = MV_STR;									
		(VAL1) = &val_xlated;										
	}													
}
