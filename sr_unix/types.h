#ifndef INC_sr_unix_H
#define INC_sr_unix_H

#define __USE_POSIX199309 1
#define __USE_XOPEN 1
#define __USE_XOPEN2K 1
#include <signal.h>

#define _SIGNAL_H 
#define __need_siginfo_t 1
#define __need_sigevent_t 1
//#define __need_siginfo_t  1
# include <bits/siginfo.h>

#define __need_sigaction_t  1
#include <bits/sigaction.h>


#include <bits/sigstack.h>

//typedef int fd_set ;
#include "sys/select.h"

typedef __sigset_t sigset_t ;

#endif
