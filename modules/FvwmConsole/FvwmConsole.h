/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <sys/types.h>

#if HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h>
#endif

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include <libs/fvwmlib.h>
#include <libs/Module.h>

#define S_NAME  "/.FvwmConsole-Socket"
/* Prompts for readline support */
#define PS1     ""
#define PS2     ">"

#define HISTSIZE 100    /* readline history file size */
#define HISTFILE "/.FvwmConsole-History"

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
