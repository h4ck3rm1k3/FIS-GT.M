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

#ifndef __ZSHOW_H__
#define __ZSHOW_H__

#define ZSHOW_DEVICE 	1
#define ZSHOW_GLOBAL 	2
#define ZSHOW_LOCAL 	3
#define ZSHOW_NOPARM	-1
#define ZSHOW_ALL	"IVBDLSC"

#define CLEANUP_ZSHOW_BUFF				\
{							\
	GBLREF char	*zwr_output_buff;		\
	if (NULL != zwr_output_buff)			\
	{						\
		free(zwr_output_buff);			\
		zwr_output_buff = NULL;			\
	}						\
}

typedef struct {
	struct lv_val_struct		*lvar;		/* local variable to output to			*/
	struct lv_val_struct		*child;		/* output variable with function subscript added*/
}zs_lv_struct;

typedef struct {
	int		end;		/* gv_currkey->end for global output variable	*/
	int		prev;		/* gv_currkey->prev 				*/
}zs_gv_struct;

typedef struct {
    char		type;		/* device, local variable or global variable	*/
    char		code;		/* function = "BDSW"				*/
    char		curr_code;	/* code from previous write			*/
    char		*buff;		/* output buffer				*/
    char		*ptr;		/* end of current output line in output buffer	*/
    int			len;		/* max line length, also size of output buffer  */
    int			line_num;	/* index for output variable starts at one	*/
    union
    {		zs_lv_struct	lv;
		zs_gv_struct	gv;
    }out_var;
    bool		flush;		/* flush the buffer				*/
}zshow_out;

#include "mlkdef.h"

void zshow_stack(zshow_out *output);
void zshow_devices(zshow_out *output);
void zshow_format_lock(zshow_out *output, mlk_pvtblk *temp);
void zshow_locks(zshow_out *output);
void zshow_output(zshow_out *out, mstr *str);
void zshow_svn(zshow_out *output);
void zshow_zbreaks(zshow_out *output);
void zshow_zcalls(zshow_out *output);
void zshow_zwrite(zshow_out *output);
boolean_t zwr2format(mstr *src, mstr *des);
int format2zwr(sm_uc_ptr_t src, int src_len, unsigned char *des, int *des_len);
void mval_write(zshow_out *output, mval *v, bool flush);
void mval_nongraphic(zshow_out *output, char *cp, int len, int num);
void gvzwr_fini(zshow_out *out, int pat);
void lvzwr_fini(zshow_out *out, int t);

#endif
