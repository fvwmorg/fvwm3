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

/*
** FShape.c: drop in replacements for the X shape library encapsulation
*/

#include "config.h"

#ifdef SHAPE

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "FShape.h"

int FShapeEventBase = 0;
int FShapeErrorBase = 0;
Bool FShapesSupported = False;

void FShapeInit(Display *dpy)
{
  FShapesSupported =
    FShapeQueryExtension(dpy, &FShapeEventBase, &FShapeErrorBase);
}

#endif /* SHAPE */
