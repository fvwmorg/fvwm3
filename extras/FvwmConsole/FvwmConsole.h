#ifdef ISC
#include <sys/bsdtypes.h> 
#endif 


#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

#include <stdio.h>
#include <sys/types.h>
#if defined linux
#include <linux/time.h>
#endif
#include <sys/un.h>
#include <sys/socket.h>
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

