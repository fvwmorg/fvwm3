/* Copyright (C) 2001  Dominik Vogt
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
** fvwmrect.c:
** This file supplies routines for fvwm internal rectangle handling.
*/

/* ---------------------------- included header files ----------------------- */

#include "config.h"
#include "fvwmrect.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

/* ---------------------------- local functions ----------------------------- */

/* ---------------------------- interface functions ------------------------- */

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
