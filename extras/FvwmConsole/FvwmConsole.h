/*
#ifdef ISC
#include <sys/bsdtypes.h> 
#endif 
*/

#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#if defined __linux
#include <linux/time.h>
#endif
#include <sys/un.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "../../config.h"
#include "../../libs/fvwmlib.h"     
#include "../../fvwm/module.h"

#define S_NAME  "/.FvConSocket"
/* Prompts for readline support */
#define PS1     ""
#define PS2     ">"
 
#define HISTSIZE 50    /* readline history file size */
#define HISTFILE "/.FvConHist"

/* #define M_PASS M_ERROR */
#define M_PASS M_ERROR

/* number of default arguments when invoked from fvwm */
#define FARGS 6   

#define XTERM "xterm"

/* message to client */
#define C_BEG   "_C_Config_Line_Begin_\n"
#define C_END   "_C_Config_Line_End_\n"
#define C_CLOSE "_C_Socket_Close_\n"

#define MAX_COMMAND_SIZE 1000
#define MAX_MESSAGE_SIZE 260
