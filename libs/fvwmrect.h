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

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global definitions -------------------------- */

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

typedef struct
{
	int x;
	int y;
	int width;
	int height;
} rectangle;

typedef struct
{
	int x;
	int y;
} position;

typedef struct
{
	int width;
	int height;
} size_rect;

typedef struct
{
	size_rect top_left;
	size_rect bottom_right;
	size_rect total_size;
} size_borders;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

/* Returns 1 if the given rectangles intersect and 0 otherwise */
int fvwmrect_do_rectangles_intersect(rectangle *r, rectangle *s);
