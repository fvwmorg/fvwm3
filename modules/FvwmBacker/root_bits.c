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
#include "libs/log.h"
#include "libs/fvwm_x11.h"
#include <stdio.h>

extern Display *dpy;
extern int screen;
extern char *Module;

unsigned long BackerGetColor(char *name)
{
  XColor color;

  color.pixel = 0;
  if (!XParseColor (dpy, DefaultColormap(dpy,screen), name, &color))
    {
      fvwm_debug(Module, "%s:  unknown color \"%s\"\n",__func__,name);
      exit(1);
    }
  else if(!XAllocColor (dpy, DefaultColormap(dpy,screen), &color))
    {
      fvwm_debug(Module, "%s:  unable to allocate color for \"%s\"\n",
                 __func__, name);
      exit(1);
    }

  return color.pixel;
}
