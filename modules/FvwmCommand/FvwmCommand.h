/* -*-c-*- */
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include "libs/ftime.h"
#include <sys/wait.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <errno.h>

#include <libs/Module.h>
#include <libs/fvwmlib.h>
#include <fvwm/fvwm.h>
#include <libs/vpacket.h>
#include <libs/fvwm_sys_stat.h>

#ifndef HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(_e)    (sys_errlist[_e])
#endif


#define F_NAME  "FvwmCommand-"

/* number of default arguments when invoked from fvwm */
#define FARGS 6

#define SOL  sizeof( unsigned long )

#ifndef HAVE_MKFIFO
#define mkfifo(path, mode) ((errno = ENOSYS) - ENOSYS - 1)
#endif


char * fifos_get_default_name();
