#ifndef INC_filestruct_h
#define INC_filestruct_h
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

#ifndef dev_t
#include <sys/types.h>
#endif

#define GDS_LABEL 	"GDSDYNUNX02" /* This string must be of length GDS_LABEL_SZ */
#define GDS_RPL_LABEL 	"GDSRPLUNX02" /* This string must be of length GDS_LABEL_SZ */

//typedef int key_t;
#ifndef   key_t
typedef int key_t;
#endif



typedef struct unix_db_info_struct
{	sgmnt_addrs	s_addrs;
	char		*fn;
	int		fd;
	gd_id		fileid;
	int		semid;
	time_t		sem_ctime;
	int		shmid;
	time_t		shm_ctime;
	int		ftok_semid;
	boolean_t	grabbed_ftok_sem;
        key_t           key;
	bool		raw;
} unix_db_info;

typedef struct unix_file_info_struct
{	int	file;
	int	fn_len;
	char	*fn;
} unix_file_info;

#define FILE_INFO(reg)	((unix_db_info *)(reg)->dyn.addr->file_cntl->file_info)
#define FILE_ID(reg)	((unix_db_info *)(reg)->dyn.addr->file_cntl->file_info)->fileid
#define GDS_INFO unix_db_info
#define MM_ADDR(x) (x->master_map)
#define FI_FN(file_info)	((unix_file_info *)(file_info))->fn
#define FI_FN_LEN(file_info)	((unix_file_info *)(file_info))->fn_len

#if defined(__osf__) || defined(_AIX)
#define REG_EQUAL(fileinfo,reg) ((fileinfo)->fileid.inode == FILE_INFO(reg)->fileid.inode		\
					&& (fileinfo)->fileid.device == FILE_INFO(reg)->fileid.device	\
					&& (fileinfo)->fileid.st_gen == FILE_INFO(reg)->fileid.st_gen)
#else
#define REG_EQUAL(fileinfo,reg) ((fileinfo)->fileid.inode == FILE_INFO(reg)->fileid.inode		\
					&& (fileinfo)->fileid.device == FILE_INFO(reg)->fileid.device)
#endif

#define WRT_STRT_PNDNG (unsigned short)65534 /* the code assumes this is non-zero, even,
					and that VMS never uses its value for iosb.cond */

#endif
