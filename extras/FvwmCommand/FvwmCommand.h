#include "config.h"
#include "fvwmlib.h"

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <signal.h>
#include <errno.h>
#include <string.h>

#include "../../fvwm/module.h"

#ifndef HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(_e)    (sys_errlist[_e])
#endif


#define F_NAME  ".FvwmCommand"

#define MAX_COMMAND_SIZE   768

#define CMD_KILL_NOUNLINK "#@killme\n"
#define CMD_CONNECT       "#@connect"
#define CMD_EXIT          "#@exit\n"

/* number of default arguments when invoked from fvwm2 */
#define FARGS 6

#define SOL  sizeof( unsigned long )

#define MYVERSION "1.5"
