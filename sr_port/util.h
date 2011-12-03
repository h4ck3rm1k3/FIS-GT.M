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

#ifndef UTIL_included
#define UTIL_included

boolean_t util_is_log_open(void);

#ifdef VMS
#include <descrip.h>

/* While the maximum for OPER_LOG_SIZE is 1906 (found from experimentation), we
   set it's maximum to 1904 being the largest 4 byte aligned size we can use. This
   4 byte aligned size is necessary to make the header length calculation done as
   sizeof(oper) - sizeof(oper.text) work correctly. If a size is used that is not
   4 byte aligned, this calculation will incorrectly contain the compiler pad chars
   causing garbage at the end of operator log lines. SE 9/2001
*/
#define	OPER_LOG_SIZE 1904

typedef struct
{
	unsigned int	req_code : 08;
	unsigned int	target   : 24;
	uint4		mess_code;
	char		text[OPER_LOG_SIZE];
} oper_msg_struct;

void util_in_open(struct dsc$descriptor_s *file_prompt);
void util_out_open(struct dsc$descriptor_s *file_prompt);
void util_log_open(char *filename, uint4 len);
#else
void util_in_open(void *);
#endif

#define OUT_BUFF_SIZE	2048
#define	NOFLUSH		0
#define FLUSH		1
#define RESET		2
#define OPER		4
#define SPRINT		5
#define HEX8		8
#define HEX16		16

void util_cm_print();
void util_exit_handler(void);
void util_out_close(void);
void util_out_print();
void util_out_send_oper(char *addr, unsigned int len);
void util_out_write(unsigned char *addr, unsigned int len);

#endif /* UTIL_included */
