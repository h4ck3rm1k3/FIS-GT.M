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

#include "mdef.h"

#include "gtm_string.h"

#include "io.h"
#include "ident.h"
#include "mmemory.h"

GBLREF io_log_name *io_root_log_name;

#define LOGNAME_LEN 255

io_log_name *get_log_name(mstr *v, bool insert)
{
        io_log_name	*l, *prev, *new_obj;
        int4		stat;
        short		index, v_len;
        unsigned char	buf[LOGNAME_LEN];
        error_def	(ERR_INVSTRLEN);

        assert (io_root_log_name != 0);
        assert(io_root_log_name->len == 0);
        v_len = v->len;
        if (v_len == 0)
	return io_root_log_name;
        if (v_len > LOGNAME_LEN)
        	rts_error(VARLSTCNT(4) ERR_INVSTRLEN, 2, v_len, LOGNAME_LEN);
	CONVERT_IDENT(buf, v->addr, v_len);
        for (prev = io_root_log_name, l = prev->next;  l != 0;  prev = l, l = l->next)
        {
               	stat = memvcmp(l->dollar_io, l->len, buf, v_len);
        	if (stat == 0)
        		return l;
        	if (stat > 0)
        		break;
        }
        if (insert == INSERT)
        {
                assert(prev != 0);
                new_obj =(io_log_name *) malloc(sizeof(*new_obj) + v_len);
                memset(new_obj, 0, sizeof(*new_obj) - 1);
                new_obj->len = v_len;
                memcpy(new_obj->dollar_io, buf, v_len);
		new_obj->dollar_io[v_len] = 0;
                prev->next = new_obj;
                new_obj->next = l;
                return new_obj;
        }
       	assert(insert == NO_INSERT);
       	return 0;
}
