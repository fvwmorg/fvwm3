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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

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

#include <errno.h>
#include <string.h>

#include <libs/Module.h>
#include <libs/fvwmlib.h>
#include <fvwm/fvwm.h>
#include <libs/vpacket.h>

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
