/* File:    debug.c
 *
 * Description:
 *      Implement some debugging/log functions that can be used generically
 *      by all of fvwm + modules.
 *
 * Created:
 *       6 Nov 1998 - Paul D. Smith <psmith@BayNetworks.com>
 */

#include "config.h"

#include <stdio.h>
#include <stdarg.h>

#include "fvwmlib.h"

#ifndef HAVE_VFPRINTF
# define VA_PRINTF(fp, lastarg, args) _doprnt((lastarg), (args), (fp))
#else
# define VA_PRINTF(fp, lastarg, args) vfprintf((fp), (lastarg), (args))
#endif

/* Don't put this into the #ifdef, since some compilers don't like completely
 * empty source files.
 */
int f_db_level = 0;


#ifdef DEBUG

struct f_db_info f_db_info;

void
f_db_print(const char *fmt, ...)
{
  va_list ap;

  fprintf(stderr, "%s:%ld: ", f_db_info.filenm, f_db_info.lineno);

  va_start(ap, fmt);
  VA_PRINTF(stderr, fmt, ap);
  va_end(ap);

  fputc('\n', stderr);
}

#endif
