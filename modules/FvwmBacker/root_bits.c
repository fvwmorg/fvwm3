/* Rewrite of this file by Dominik Vogt on Nov-1-1998 to remove the
 * Xconsortium copyright.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <X11/Xlib.h>
#include <stdio.h>

extern Display *dpy;
extern int screen;
extern char *Module;

unsigned long GetColor(char *name)
{
  XColor color;

  color.pixel = 0;
  if (!XParseColor (dpy, DefaultColormap(dpy,screen), name, &color))
    {
      fprintf(stderr,"%s:  unknown color \"%s\"\n",Module,name);
      exit(1);
    }
  else if(!XAllocColor (dpy, DefaultColormap(dpy,screen), &color))
    {
      fprintf(stderr, "%s:  unable to allocate color for \"%s\"\n",
	      Module, name);
      exit(1);
    }

  return color.pixel;
}
