/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

/* File:    debug.c
 *
 * Description:
 *	Implement some debugging/log functions that can be used generically
 *	by all of fvwm + modules.
 *
 * Created:
 *	 6 Nov 1998 - Paul D. Smith <psmith@BayNetworks.com>
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

/* Global variable filled by callers of the DB library.	 */
struct f_db_info f_db_info;

static int is_db_initialized = 0;
static FILE *f_db_file;

/* Initializes the DB library.	You can't make any DB calls until this
 * function has been called.
 */
static void
f_db_init()
{
  const char *evp;
  int err = 0;

  f_db_file = stderr;

  if ((evp = getenv(FVWM_DB_FILE)) != NULL) {
    FILE *fp;

    if ((fp = fopen(evp, "w")) == NULL)
      err = errno;
    else
      f_db_file = fp;
  }

#ifdef HAVE_SETVBUF
# ifdef SETVBUF_REVERSED
  setvbuf(f_db_file, NULL, BUFSIZ, _IOLBF);
# else
  setvbuf(f_db_file, NULL, _IOLBF, BUFSIZ);
# endif
#endif

  if (err)
    fprintf(stderr, "FVWM f_db_init(): %s\n", strerror(err));

  if ((evp = getenv(FVWM_DB_LEVEL)) != NULL) {
    int lev;

    if (sscanf(evp, "%d", &lev) != 1)
      fprintf(stderr,
	      "FVWM f_db_init(): Invalid value for " FVWM_DB_LEVEL ": %s\n",
	      evp);
    else
      f_db_level = lev;
  }

  is_db_initialized = 1;
}

void
f_db_print(const char *fmt, ...)
{
  va_list ap;

  if (!is_db_initialized)
    f_db_init();

  if (f_db_info.filenm) {
    fputs(f_db_info.filenm, stderr);
    fputc(':', stderr);
  }

  if (f_db_info.lineno < 0)
    fprintf(stderr, "%ld:", f_db_info.lineno);

  if (f_db_info.module) {
    fputs(f_db_info.module, stderr);
    fputc(':', stderr);
  }

  if (f_db_info.funcnm)
    fputs(f_db_info.funcnm, stderr);
    fputc(':', stderr);
  }

  fputc(' ', stderr);

  va_start(ap, fmt);
  VA_PRINTF(stderr, fmt, ap);
  va_end(ap);

  fputc('\n', stderr);
}

#endif
