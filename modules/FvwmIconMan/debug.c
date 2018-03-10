/* -*-c-*- */
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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include <stdarg.h>
#include <assert.h>

#include "FvwmIconMan.h"

static FILE *console = NULL;

/* I'm finding lots of the debugging is dereferencing pointers
   to zero.  I fixed some of them, until I grew tired of the game.
   If you want to turn these back on, be prepared for lots of core
   dumps.  dje 11/15/98. */
int CORE = 0;
int FUNCTIONS = 0;
int X11 = 0;
int FVWM = 0;
int CONFIG = 0;
int WINLIST = 0;

void
ConsoleMessage(const char *fmt, ...)
{
	char *mfmt;
	va_list args;

	assert(console != NULL);

	fputs("FvwmIconMan: ", console);

	va_start(args, fmt);
	{
		int n;

		n = asprintf(&mfmt, "%s\n", fmt);
		(void)n;
	}
	vfprintf(console, mfmt, args);
	va_end(args);
	free(mfmt);
}

int
OpenConsole(const char *filenm)
{
	if (!filenm)
	{
		console = stderr;
	}
	else if ((console = fopen(filenm, "w")) == NULL)
	{
		fprintf(stderr,"%s: cannot open %s\n", MyName, filenm);
		return 0;
	}

	return 1;
}

void
ConsoleDebug(int flag, const char *fmt, ...)
{
	assert(console != NULL);

#ifdef FVWM_DEBUG_MSGS
	if (flag)
	{
		va_list args;

		va_start(args, fmt);
		vfprintf(console, fmt, args);
		fflush(console);
		va_end(args);
	}
#endif
}
