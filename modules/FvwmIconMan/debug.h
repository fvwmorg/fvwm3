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

#ifndef IN_DEBUG_H
#define IN_DEBUG_H

#if 0
# define PRINT_DEBUG
#endif

#if 0
# define OUTPUT_FILE "/dev/console"
#else
# define OUTPUT_FILE NULL
#endif


extern int OpenConsole(const char *filenm);
extern void ConsoleMessage(const char *fmt, ...)
			   __attribute__ ((__format__ (__printf__, 1, 2)));
extern void ConsoleDebug(int flag, const char *fmt, ...)
			 __attribute__ ((__format__ (__printf__, 2, 3)));

extern int CORE, FUNCTIONS, X11, FVWM, CONFIG, WINLIST, MEM;

#endif
