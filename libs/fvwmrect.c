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
    return 0;
  if (s->x + s->width <= r->x)
    return 0;
  if (r->y + r->height <= s->y)
    return 0;
  if (s->y + s->height <= r->y)
    return 0;

  return 1;
}
