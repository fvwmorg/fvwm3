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

#ifndef IN_DEBUG_H
#define IN_DEBUG_H

#if 0
#define PRINT_DEBUG      
#endif

#if 1
#define OUTPUT_FILE "/dev/console"
#else
#define OUTPUT_FILE "/usr/bradym/log"
#endif


#if !defined (PRINT_DEBUG) && defined (__GNUC__)
#define ConsoleDebug(flag, fmt, args...)
#else
extern void ConsoleDebug(int flag, char *fmt, ...);
#endif

extern int OpenConsole (void);

#define DEBUG_ALWAYS 1

extern int CORE, FUNCTIONS, X11, FVWM, CONFIG, WINLIST, MEM;

#endif
