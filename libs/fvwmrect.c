/* -*-c-*- */
/* Copyright (C) 2001  Dominik Vogt */
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
** fvwmrect.c:
** This file supplies routines for fvwm internal rectangle handling.
*/

/* ---------------------------- included header files ---------------------- */

#include "config.h"
#include <X11/Xlib.h>
#include "fvwmrect.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- local functions ---------------------------- */

static int fvwmrect_do_intervals_intersect(
	int x1, int width1, int x2, int width2)
{
	return !(x1 + width1 <= x2 || x2 + width2 <= x1);
}

/* ---------------------------- interface functions ------------------------ */

/* Returns 1 if the given rectangles intersect and 0 otherwise */
int fvwmrect_do_rectangles_intersect(rectangle *r, rectangle *s)
{
	if (r->x + r->width <= s->x)
	{
		return 0;
	}
	if (s->x + s->width <= r->x)
	{
		return 0;
	}
	if (r->y + r->height <= s->y)
	{
		return 0;
	}
	if (s->y + s->height <= r->y)
	{
		return 0;
	}

	return 1;
}

/* Subtracts the values in s2_ from the ones in s1_g and stores the result in
 * diff_g. */
void fvwmrect_subtract_rectangles(
	rectangle *rdiff, rectangle *r1, rectangle *r2)
{
	rdiff->x = r1->x - r2->x;
	rdiff->y = r1->y - r2->y;
	rdiff->width = r1->width - r2->width;
	rdiff->height = r1->height - r2->height;

	return;
}

/* Returns 1 is the rectangles are identical and 0 if not */
int fvwmrect_rectangles_equal(
	rectangle *r1, rectangle *r2)
{
	if (r1 == r2)
	{
		return 1;
	}
	if (r1 == NULL || r2 == NULL)
	{
		return 0;
	}
	if (r1->x != r2->x || r1->y != r2->y || r1->width != r2->width ||
	    r1->height != r2->height)
	{
		return 1;
	}

	return 0;
}

int fvwmrect_move_into_rectangle(rectangle *move_rec, rectangle *target_rec)
{
	int has_changed = 0;

	if (!fvwmrect_do_intervals_intersect(
		    move_rec->x, move_rec->width, target_rec->x,
		    target_rec->width))
	{
		move_rec->x = move_rec->x % (int)target_rec->width;
		if (move_rec->x < 0)
		{
			move_rec->x += target_rec->width;
		}
		move_rec->x += target_rec->x;
		has_changed = 1;
	}
	if (!fvwmrect_do_intervals_intersect(
		    move_rec->y, move_rec->height, target_rec->y,
		    target_rec->height))
	{
		move_rec->y = move_rec->y % (int)target_rec->height;
		if (move_rec->y < 0)
		{
			move_rec->y += target_rec->height;
		}
		move_rec->y += target_rec->y;
		has_changed = 1;
	}

	return has_changed;
}

int fvwmrect_intersect_xrectangles(
	XRectangle *r1, XRectangle *r2)
{
	int x1 = max(r1->x, r2->x);
	int y1 = max(r1->y, r2->y);
	int x2 = min(r1->x + r1->width, r2->x + r2->width);
	int y2 = min(r1->y + r1->height, r2->y + r2->height);

	r1->x = x1;
	r1->y = y1;
	r1->width = x2 - x1;
	r1->height = y2 - y1;

	if (x2 > x1 && y2 > y1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
