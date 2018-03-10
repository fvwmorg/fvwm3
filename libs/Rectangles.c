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

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <X11/Xlib.h>

#include "Rectangles.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- interface functions ------------------------ */

Bool frect_get_intersection(
	int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2,
	XRectangle *r)
{
	if (x1 + w1 > x2 && x1 < x2 + w2 &&
	    y1 + h1 > y2 && y1 < y2 + h2)
	{
		if (r)
		{
			r->x = max(x1,x2);
			r->y = max(y1,y2);
			r->width = min(x1+w1,x2+w2) - max(x1,x2);
			r->height =min(y1+h1,y2+h2) - max(y1,y2);
		}
		return True;
	}
	return False;
}


Bool frect_get_seg_intersection(
	int x1, int w1, int x2, int w2, int *x, int *w)
{
	if (x1 + w1 > x2 && x1 < x2 + w2)
	{
		if (x)
		{
			*x = max(x1,x2);
		}
		if (w)
		{
			*w = min(x1+w1,x2+w2) - max(x1,x2);
		}
		return True;
	}
	return False;
}

Bool frect_get_rect_intersection(
	XRectangle a, XRectangle b, XRectangle *r)
{
	return frect_get_intersection(
		a.x, a.y, a.width, a.height, b.x, b.y, b.width, b.height, r);
}
