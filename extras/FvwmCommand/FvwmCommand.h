
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

#if defined __linux
#include <linux/time.h>
#include <getopt.h>
#endif

#include <signal.h>
#include <errno.h>
#include <string.h>

#include "../../configure.h"
#include "../../version.h"
#include "../../libs/fvwmlib.h"     
#include "../../fvwm/module.h"
#if defined (sparc) && !defined (SVR4)
#include "sunos_headers.h" 
#include "../../fvwm/sun_headers.h"
#endif

#define F_NAME  "/.FvwmCommand"

#define MAX_COMMAND_SIZE   768

/* number of default arguments when invoked from fvwm */
#define FARGS 6   

#define SOL  sizeof( unsigned long )

#define MYVERSION "1.4"
