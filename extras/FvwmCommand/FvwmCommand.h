
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif


#ifdef __hpux
#define  _select( s, fd, r, w, t ) \
			select(s,(int *)fd, r, w, t)
#else
#define	 _select(s, fd, r, w, t) \
			select(s, fd, r, w, t)
#endif  

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>

#if defined __linux
#include <getopt.h>
#endif

#include <signal.h>
#include <errno.h>
#include <string.h>

#include "../../config.h"
#include "../../libs/fvwmlib.h"     
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
