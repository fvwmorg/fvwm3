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

#include <stdarg.h>
#include <assert.h>

#include "FvwmIconMan.h"

#include "debuglevels.h"

static char const rcsid[] =
  "$Id$";

static FILE *console = NULL;

void
ConsoleMessage(const char *fmt, ...)
{
  va_list args;

  assert(console != NULL);

  fputs("FvwmIconMan: ", console);

  va_start(args, fmt);
  vfprintf(console, fmt, args);
  va_end(args);
}

int
OpenConsole(const char *filenm)
{
  if (!filenm)
    console = stderr;
  else if ((console = fopen(filenm, "w")) == NULL) {
    fprintf(stderr,"%s: cannot open %s\n", Module, filenm);
    return 0;
  }

  return 1;
}

void
ConsoleDebug(int flag, const char *fmt, ...)
{
  assert(console != NULL);

#ifdef PRINT_DEBUG
  if (flag) {
    va_list args;

    va_start(args, fmt);
    vfprintf(console, fmt, args);
    fflush(console);
    va_end(args);
  }
#endif
}
