/* Copyright (C) 1999  Dominik Vogt
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This function should be used by all modules and fvwm when a X error
 * occurs and the module exits.
 */

#include <stdio.h>
#include <X11/Xlib.h>

void PrintXErrorAndCoredump(Display *dpy, XErrorEvent *error, char *MyName)
{
  char msg[256];

  XGetErrorText(dpy, error->error_code, msg, 256);

  fprintf(stderr,"%s: Cause of next X Error.\n", MyName);
  fprintf(stderr, "   Error: %d (%s)\n", error->error_code, msg);
  fprintf(stderr, "   Major opcode of failed request:  %d \n",
	  error->request_code);
  fprintf(stderr, "   Minor opcode of failed request:  %d \n",
	  error->minor_code);
  fprintf(stderr, "   Resource id of failed request:  0x%lx \n",
	  error->resourceid);
  fprintf(stderr, " Leaving a core dump now\n");

  /* leave a coredump */
  abort();
  /* exit if this fails */
  exit(99);
}
