#include "config.h"
#include "fvwmlib.h"

#if HAVE_SYS_SELECT_H
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

#include <signal.h>
#include <errno.h>
#include <string.h>

#include "../../fvwm/module.h"

#define F_NAME  "/.FvwmCommand"

#define MAX_COMMAND_SIZE   768

/* number of default arguments when invoked from fvwm */
#define FARGS 6

#define SOL  sizeof( unsigned long )

#define MYVERSION "1.4"
