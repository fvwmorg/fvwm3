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
