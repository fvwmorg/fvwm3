#include "config.h"

#if HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h>
#endif

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "fvwmlib.h"
#include "../../fvwm/module.h"

#define S_NAME "/tmp/FvConSocket"
#define BUFSIZE 511   /* maximum error message size */
#define NEWLINE "\n"

/* timeout for packet after issuing command*/
#define TIMEOUT 400000

/* #define M_PASS M_ERROR */

#define M_PASS M_ERROR

/* number of default arguments when invoked from fvwm */
#define FARGS 6

#define XTERM "xterm"

/* message to client */
#define C_BEG "_C_Config_Line_Begin_\n"
#define C_END "_C_Config_Line_End_\n"
#define C_CLOSE "_C_Socket_Close_\n"

