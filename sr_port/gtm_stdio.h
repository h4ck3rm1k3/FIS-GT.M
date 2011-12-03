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

/* gtm_stdio.h - gtm interface to stdio.h */

#ifndef GTM_STDIOH
#define GTM_STDIOH

#include <stdio.h>

#define FDOPEN				fdopen
#define FGETS(strg,n,strm,fgets_res)	(fgets_res = fgets(strg,n,strm))
#define Fopen				fopen
#define GETS(buffer,gets_res)		syntax error
#define PERROR				perror
#define	POPEN				popen
#define TEMPNAM				tempnam
#define RENAME				rename
#define SETVBUF				setvbuf
#define FPRINTF         		fprintf
#define FSCANF         			fscanf
#define PRINTF         			printf
#define SCANF          			scanf
#define SSCANF         			sscanf
#define SPRINTF       			sprintf
#ifdef VMS
int					gtm_snprintf(char *str, size_t size, const char *format, ...);
#define SNPRINTF       			gtm_snprintf /* hack for VMS, ignore size argument and call sprintf */
#else
#define SNPRINTF       			snprintf
#endif
#define VFPRINTF       			vfprintf
#define VPRINTF        			vprintf
#define VSPRINTF       			vsprintf

#define SPRINTF_ENV_NUM(BUFF, ENV_VAR, ENV_VAL, ENV_IND)							\
{														\
	assert(NULL == strchr(ENV_VAR, '='));	/* strchr() done later in ojstartchild() relies on this */	\
	sprintf(BUFF, "%s=%d", ENV_VAR, ENV_VAL); *ENV_IND++ = BUFF;						\
}

#define SPRINTF_ENV_STR(BUFF, ENV_VAR, ENV_VAL, ENV_IND)							\
{														\
	assert(NULL == strchr(ENV_VAR, '='));	/* strchr() done later in ojstartchild() relies on this */	\
	sprintf(BUFF, "%s=%s", ENV_VAR, ENV_VAL); *ENV_IND++ = BUFF;						\
}

#endif
