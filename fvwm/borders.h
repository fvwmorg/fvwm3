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


#define NPARTS_KEEP_STATE 12

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

typedef enum
{
	PART_NONE      = 0x0,
	/* the border drawing code relies on the fact that the border bits
	 * occupy the lowest ten bits in this order! */
	PART_BORDER_N  = 0x1,
	PART_BORDER_S  = 0x2,
	PART_BORDER_E  = 0x4,
	PART_BORDER_W  = 0x8,
	PART_BORDER_NW = 0x10,
	PART_BORDER_NE = 0x20,
	PART_BORDER_SW = 0x40,
	PART_BORDER_SE = 0x80,
	PART_TITLE     = 0x100,
	PART_BUTTONS   = 0x200,
	PART_X_HANDLES = 0x400,
	PART_Y_HANDLES = 0x800,
	/* combinations of the above values */
	PART_SIDES     = 0x0f,
	PART_CORNERS   = 0xf0,
	PART_FRAME     = 0xff,
	PART_TITLEBAR  = 0x300,
	PART_HANDLES   = 0xc00,
	PART_ALL       = 0xfff
} window_parts;

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

DecorFace *border_get_border_style(
	FvwmWindow *fw, Bool has_focus);
int border_is_using_border_style(
	FvwmWindow *fw, Bool has_focus);
int border_context_to_parts(
	int context);
void border_get_part_geometry(
	FvwmWindow *fw, window_parts part, rectangle *sidebar_g,
	rectangle *ret_g, Window *ret_w);
int get_button_number(int context);
void border_draw_decorations(
	FvwmWindow *t, window_parts draw_parts, Bool has_focus, int force,
	clear_window_parts clear_parts, rectangle *old_g, rectangle *new_g);
void border_undraw_decorations(
	FvwmWindow *fw);
void border_redraw_decorations(
	FvwmWindow *fw);

#endif /* _BORDERS_H */
