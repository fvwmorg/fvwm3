/*
 * This program is free software; you can redistribute it and/or modify
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

#include <stdarg.h>
#include "FvwmIconMan.h"

#include "debuglevels.h"

static char const rcsid[] =
  "$Id$";

static FILE *console = NULL;

int OpenConsole(void)
{
#ifdef OUTPUT_FILE
  if ((console=fopen(OUTPUT_FILE, "w"))==NULL) {
    fprintf(stderr,"%s: cannot open %s\n", Module, OUTPUT_FILE);
    return 0;
  }
#endif
  return 1;
}

void ConsoleMessage(char *fmt, ...)
{
  va_list args;
  FILE *filep;

  if (console==NULL) 
    filep=stderr;
  else 
    filep=console;
  
  fprintf (filep, "FvwmIconMan: ");
  va_start(args,fmt);
  vfprintf(filep,fmt,args);
  va_end(args);
}

#if defined(PRINT_DEBUG) || !defined(__GNUC__)
void ConsoleDebug (int flag, char *fmt, ...) {
#ifdef PRINT_DEBUG
  va_list args;
  FILE *filep;
  if (flag) {
    if (console==NULL) 
      filep=stderr;
    else 
      filep=console;
    va_start(args,fmt);
    vfprintf(filep,fmt,args);
    fflush(filep);
    va_end(args);
  }
#endif
}
#endif
