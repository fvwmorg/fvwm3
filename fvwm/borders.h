/* -*-c-*- */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#ifndef _BORDERS_H
#define _BORDERS_H

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global definitions -------------------------- */

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

typedef enum
{
	DRAW_NONE      = 0x0,
	/* the border drawing code relies on the fact that the border bits
	 * occupy the lowest ten bits in this order! */
	DRAW_BORDER_N  = 0x1,
	DRAW_BORDER_S  = 0x2,
	DRAW_BORDER_E  = 0x4,
	DRAW_BORDER_W  = 0x8,
	DRAW_BORDER_NW = 0x10,
	DRAW_BORDER_NE = 0x20,
	DRAW_BORDER_SW = 0x40,
	DRAW_BORDER_SE = 0x80,
	DRAW_X_HANDLES = 0x100,
	DRAW_Y_HANDLES = 0x200,
	DRAW_TITLE     = 0x400,
	DRAW_BUTTONS   = 0x800,
	DRAW_SIDES     = 0x0f,
	DRAW_CORNERS   = 0xf0,
	DRAW_FRAME     = 0xff,
	DRAW_HANDLES   = 0x300,
	DRAW_ALL       = 0xfff
} draw_window_parts;

typedef enum
{
	CLEAR_NONE     = 0x0,
	CLEAR_FRAME    = 0x1,
	CLEAR_TITLE    = 0x2,
	CLEAR_BUTTONS  = 0x4,
	CLEAR_ALL      = 0x7
} clear_window_parts;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

void border_get_part_geometry(
	FvwmWindow *fw, draw_window_parts part, rectangle *sidebar_g,
	rectangle *ret_g, Window *ret_w);
int get_button_number(int context);
void draw_clipped_decorations_with_geom(
	FvwmWindow *t, draw_window_parts draw_parts, Bool has_focus, int force,
	Window expose_win, XRectangle *rclip, clear_window_parts clear_parts,
	rectangle *old_g, rectangle *new_g);
void draw_clipped_decorations(
	FvwmWindow *t, draw_window_parts draw_parts, Bool has_focus, int force,
	Window expose_win, XRectangle *rclip, clear_window_parts clear_parts);
void DrawDecorations(
	FvwmWindow *t, draw_window_parts draw_parts, Bool has_focus, int force,
	Window expose_win, clear_window_parts clear_parts);
void RedrawDecorations(FvwmWindow *fw);

#endif /* _BORDERS_H */
