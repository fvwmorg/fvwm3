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

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include <libs/gravity.h>
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "geometry.h"
#include "module_interface.h"
#include "borders.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

/* ---------------------------- interface functions ------------------------- */

/* Removes decorations from the source rectangle and moves it according to the
 * gravity specification. */
void gravity_get_naked_geometry(
	int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g)
{
	int xoff;
	int yoff;
	size_borders b;

	get_window_borders(t, &b);
	gravity_get_offsets(gravity, &xoff, &yoff);
	dest_g->x = orig_g->x + ((xoff + 1) * (orig_g->width - 1)) / 2;
	dest_g->y = orig_g->y + ((yoff + 1) * (orig_g->height - 1)) / 2;
	dest_g->width = orig_g->width - b.total_size.width;
	dest_g->height = orig_g->height - b.total_size.height;

	return;
}

/* Decorate the rectangle.  Resize and shift it according to gravity. */
void gravity_add_decoration(
	int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g)
{
	size_borders b;

	get_window_borders(t, &b);
	*dest_g = *orig_g;
	gravity_resize(
		gravity, dest_g, b.total_size.width, b.total_size.height);

	return;
}

void get_relative_geometry(rectangle *rel_g, rectangle *abs_g)
{
	rel_g->x = abs_g->x - Scr.Vx;
	rel_g->y = abs_g->y - Scr.Vy;
	rel_g->width = abs_g->width;
	rel_g->height = abs_g->height;

	return;
}

void gravity_translate_to_northwest_geometry(
	int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g)
{
	int xoff;
	int yoff;

	gravity_get_offsets(gravity, &xoff, &yoff);
	dest_g->x = orig_g->x -
		((xoff + 1) * (orig_g->width - 1 +
			       2 * t->attr_backup.border_width)) / 2;
	dest_g->y = orig_g->y -
		((yoff + 1) * (orig_g->height - 1 +
			       2 * t->attr_backup.border_width)) / 2;
	dest_g->width = orig_g->width;
	dest_g->height = orig_g->height;

	return;
}

void gravity_translate_to_northwest_geometry_no_bw(
	int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g)
{
	int bw = t->attr_backup.border_width;

	t->attr_backup.border_width = 0;
	gravity_translate_to_northwest_geometry(gravity, t, dest_g, orig_g);
	t->attr_backup.border_width = bw;

	return;
}

void get_title_geometry(
	FvwmWindow *fw, rectangle *ret_g)
{
	size_borders b;
	size_borders nt;
	int w;
	int h;

	get_window_borders(fw, &b);
	get_window_borders_no_title(fw, &nt);
	w = (ret_g->width > 0) ? ret_g->width : fw->frame_g.width;
	h = (ret_g->height > 0) ? ret_g->height : fw->frame_g.height;
	ret_g->x = nt.top_left.width;
	ret_g->y = nt.top_left.height;
	switch (GET_TITLE_DIR(fw))
	{
	case DIR_S:
		ret_g->y = h - b.bottom_right.height;
		/* fall through */
	case DIR_N:
		ret_g->width = w - b.total_size.width;
		ret_g->height = fw->title_thickness;
		break;
	case DIR_E:
		ret_g->x = w - b.bottom_right.width;
		/* fall through */
	case DIR_W:
		ret_g->width = fw->title_thickness;
		ret_g->height = h - b.total_size.height;
		break;
	default:
		break;
	}

	return;
}

void get_title_gravity_factors(
	FvwmWindow *fw, int *ret_fx, int *ret_fy)
{
	switch (GET_TITLE_DIR(fw))
	{
	case DIR_N:
		*ret_fx = 0;
		*ret_fy = 1;
		break;
	case DIR_S:
		*ret_fx = 0;
		*ret_fy = -1;
		break;
	case DIR_W:
		*ret_fx = 1;
		*ret_fy = 0;
		break;
	case DIR_E:
		*ret_fx = -1;
		*ret_fy = 0;
		break;
	}

	return;
}

Bool get_title_button_geometry(
	FvwmWindow *fw, rectangle *ret_g, int context)
{
	int bnum;

	if (context & C_TITLE)
	{
		ret_g->width = 0;
		ret_g->height = 0;
		get_title_geometry(fw, ret_g);

		return True;

	}
	bnum = get_button_number(context);
	if (bnum < 0 || FW_W_BUTTON(fw, bnum) == None)
	{
		return False;
	}
	if (XGetGeometry(
		dpy, FW_W_BUTTON(fw, bnum), &JunkRoot, &ret_g->x, &ret_g->y,
		&ret_g->width, &ret_g->height, &JunkBW, &JunkDepth) == 0)
	{
		return False;
	}
	XTranslateCoordinates(
		dpy, FW_W_FRAME(fw), Scr.Root, ret_g->x, ret_g->y, &ret_g->x,
		&ret_g->y, &JunkChild);

	return True;
}

void get_title_font_size_and_offset(
	FvwmWindow *fw, direction_type title_dir,
	Bool is_left_title_rotated_cw, Bool is_right_title_rotated_cw,
	int *size, int *offset)
{
	int decor_size;
	int extra_size;
	int font_size;
	int min_offset;
        Bool is_rotated_cw;

	/* adjust font offset according to height specified in title style */
	decor_size = fw->decor->title_height;
	font_size = fw->title_font->height + EXTRA_TITLE_FONT_HEIGHT;
        min_offset = fw->title_font->ascent;
	switch (title_dir)
	{
	case DIR_W:
	case DIR_E:
                is_rotated_cw = (title_dir == DIR_W) ?
                        is_left_title_rotated_cw : is_right_title_rotated_cw;
		if (is_rotated_cw)
		{
			fw->title_text_rotation = TEXT_ROTATED_90;
			min_offset = fw->title_font->height -
				fw->title_font->ascent;
		}
		else
		{
			fw->title_text_rotation = TEXT_ROTATED_270;
		}
		break;
	case DIR_N:
	case DIR_S:
	default:
		fw->title_text_rotation = TEXT_ROTATED_0;
		break;
	}
	extra_size = (decor_size > 0) ? decor_size - font_size : 0;
	*offset = min_offset;
	if (extra_size > 0)
	{
		*offset += extra_size / 2;
	}
	*size = font_size + extra_size;

	return;
}

void get_icon_corner(
	FvwmWindow *fw, rectangle *ret_g)
{
	switch (GET_TITLE_DIR(fw))
	{
	case DIR_N:
	case DIR_W:
		ret_g->x = fw->frame_g.x;
		ret_g->y = fw->frame_g.y;
		break;
	case DIR_S:
		ret_g->x = fw->frame_g.x;
		ret_g->y = fw->frame_g.y + fw->frame_g.height -
			ret_g->height;
		break;
	case DIR_E:
		ret_g->x = fw->frame_g.x + fw->frame_g.width -
			ret_g->width;
		ret_g->y = fw->frame_g.y;
		break;
	}

	return;
}

void get_shaded_geometry(
	FvwmWindow *fw, rectangle *small_g, rectangle *big_g)
{
	size_borders b;
	/* this variable is necessary so the function can be called with
	 * small_g == big_g */
	int big_width = big_g->width;
	int big_height = big_g->height;

	get_window_borders(fw, &b);
	*small_g = *big_g;
	switch (SHADED_DIR(fw))
	{
	case DIR_S:
	case DIR_SW:
	case DIR_SE:
		small_g->y = big_g->y + big_height - b.total_size.height;
		/* fall through */
	case DIR_N:
	case DIR_NW:
	case DIR_NE:
		small_g->height = b.total_size.height;
		break;
	default:
		break;
	}
	switch (SHADED_DIR(fw))
	{
	case DIR_E:
	case DIR_NE:
	case DIR_SE:
		small_g->x = big_g->x + big_width - b.total_size.width;
		/* fall through */
	case DIR_W:
	case DIR_NW:
	case DIR_SW:
		small_g->width = b.total_size.width;
		break;
	default:
		break;
	}

	return;
}

void get_unshaded_geometry(
	FvwmWindow *fw, rectangle *ret_g)
{
	if (IS_SHADED(fw))
	{
		if (IS_MAXIMIZED(fw))
		{
			*ret_g = fw->max_g;
		}
		else
		{
			*ret_g = fw->normal_g;
		}
		get_relative_geometry(ret_g, ret_g);
	}
	else
	{
		*ret_g = fw->frame_g;
	}

	return;
}

void get_shaded_client_window_pos(
	FvwmWindow *fw, rectangle *ret_g)
{
	rectangle big_g;
	size_borders b;

	get_window_borders(fw, &b);
	big_g = (IS_MAXIMIZED(fw)) ? fw->max_g : fw->normal_g;
	get_relative_geometry(&big_g, &big_g);
	switch (SHADED_DIR(fw))
	{
	case DIR_S:
	case DIR_SW:
	case DIR_SE:
		ret_g->y = 1 - big_g.height + b.total_size.height;
		break;
	default:
		ret_g->y = 0;
		break;
	}
	switch (SHADED_DIR(fw))
	{
	case DIR_E:
	case DIR_NE:
	case DIR_SE:
		ret_g->x = 1 - big_g.width + b.total_size.width;
		break;
	default:
		ret_g->x = 0;
		break;
	}

	return;
}

/* returns the dimensions of the borders */
void get_window_borders(
	FvwmWindow *fw, size_borders *borders)
{
	borders->top_left.width = fw->boundary_width;
	borders->bottom_right.width = fw->boundary_width;
	borders->top_left.height = fw->boundary_width;
	borders->bottom_right.height = fw->boundary_width;
	switch (GET_TITLE_DIR(fw))
	{
	case DIR_N:
		borders->top_left.height += fw->title_thickness;
		break;
	case DIR_S:
		borders->bottom_right.height += fw->title_thickness;
		break;
	case DIR_W:
		borders->top_left.width += fw->title_thickness;
		break;
	case DIR_E:
		borders->bottom_right.width += fw->title_thickness;
		break;
	}
	borders->total_size.width =
		borders->top_left.width + borders->bottom_right.width;
	borders->total_size.height =
		borders->top_left.height + borders->bottom_right.height;

	return;
}

/* returns the dimensions of the borders without the title */
void get_window_borders_no_title(
	FvwmWindow *fw, size_borders *borders)
{
	borders->top_left.width = fw->boundary_width;
	borders->bottom_right.width = fw->boundary_width;
	borders->top_left.height = fw->boundary_width;
	borders->bottom_right.height = fw->boundary_width;
	borders->total_size.width =
		borders->top_left.width + borders->bottom_right.width;
	borders->total_size.height =
		borders->top_left.height + borders->bottom_right.height;

	return;
}

void set_window_border_size(
	FvwmWindow *fw, short used_width)
{
	if (used_width <= 0)
	{
		fw->boundary_width = 0;
	}
	else
	{
		fw->boundary_width = used_width;
	}

	return;
}

/* Returns True if all window borders are only 1 pixel thick (or less). */
Bool is_window_border_minimal(
	FvwmWindow *fw)
{
	size_borders nt;

	get_window_borders_no_title(fw, &nt);
	if (nt.top_left.width > 1 || nt.top_left.height > 1 ||
	    nt.bottom_right.width > 1 || nt.bottom_right.height > 1)
	{
		return False;
	}

	return True;
}


/* This function returns the geometry of the client window.  If the window is
 * shaded, the unshaded geometry is used instead. */
void get_client_geometry(
	FvwmWindow *fw, rectangle *ret_g)
{
	size_borders borders;

	get_unshaded_geometry(fw, ret_g);
	get_window_borders(fw, &borders);
	ret_g->x += borders.top_left.width;
	ret_g->y += borders.top_left.height;
	ret_g->width -= borders.total_size.width;
	ret_g->height -= borders.total_size.height;

	return;
}

/* update the frame_g according to the window's normal_g or max_g and shaded
 * state */
void update_relative_geometry(FvwmWindow *fw)
{
	get_relative_geometry(
		&fw->frame_g,
		(IS_MAXIMIZED(fw)) ? &fw->max_g : &fw->normal_g);
	if (IS_SHADED(fw))
	{
		get_shaded_geometry(
			fw, &fw->frame_g, &fw->frame_g);
	}

	return;
}

/* update the normal_g or max_g according to the window's current position */
void update_absolute_geometry(FvwmWindow *fw)
{
	rectangle *dest_g;

	/* store orig values in absolute coords */
	dest_g = (IS_MAXIMIZED(fw)) ? &fw->max_g : &fw->normal_g;
	dest_g->x = fw->frame_g.x + Scr.Vx;
	dest_g->y = fw->frame_g.y + Scr.Vy;
	dest_g->width = fw->frame_g.width;
	if (!IS_SHADED(fw))
	{
		dest_g->height = fw->frame_g.height;
	}
	else if (SHADED_DIR(fw) == DIR_S)
	{
		dest_g->y += fw->frame_g.height - dest_g->height;
	}

	return;
}

/* make sure a maximized window and it's normal version are never a page or
 * more apart. */
void maximize_adjust_offset(FvwmWindow *fw)
{
	int off_x;
	int off_y;

	if (!IS_MAXIMIZED(fw))
	{
		/* otherwise we might corrupt the normal_g */
		return;
	}
	off_x = fw->normal_g.x - fw->max_g.x - fw->max_offset.x;
	off_y = fw->normal_g.y - fw->max_g.y - fw->max_offset.y;
	if (off_x >= Scr.MyDisplayWidth)
	{
		fw->normal_g.x -=
			(off_x / Scr.MyDisplayWidth) * Scr.MyDisplayWidth;
	}
	else if (off_x <= -Scr.MyDisplayWidth)
	{
		fw->normal_g.x +=
			((-off_x) / Scr.MyDisplayWidth) * Scr.MyDisplayWidth;
	}
	if (off_y >= Scr.MyDisplayHeight)
	{
		fw->normal_g.y -=
			(off_y / Scr.MyDisplayHeight) * Scr.MyDisplayHeight;
	}
	else if (off_y <= -Scr.MyDisplayHeight)
	{
		fw->normal_g.y +=
			((-off_y) / Scr.MyDisplayHeight) * Scr.MyDisplayHeight;
	}

	return;
}

/***********************************************************************
 *
 *  Procedure:
 *      constrain_size - adjust the given width and height to account for the
 *              constraints imposed by size hints
 *
 *      The general algorithm, especially the aspect ratio stuff, is
 *      borrowed from uwm's CheckConsistency routine.
*
***********************************************************************/
#define MAKEMULT(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
void constrain_size(
	FvwmWindow *fw, unsigned int *widthp, unsigned int *heightp,
	int xmotion, int ymotion, int flags)
{
	int minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
	int baseWidth, baseHeight;
	int dwidth = *widthp, dheight = *heightp;
	int roundUpX = 0;
	int roundUpY = 0;
	int old_w = 0;
	int old_h = 0;
	size_borders b;

	if (IS_MAXIMIZED(fw) && (flags & CS_UPDATE_MAX_DEFECT))
	{
		*widthp += fw->max_g_defect.width;
		*heightp += fw->max_g_defect.height;
		old_w = *widthp;
		old_h = *heightp;
	}
	get_window_borders(fw, &b);
	dwidth -= b.total_size.width;
	dheight -= b.total_size.height;

	minWidth = fw->hints.min_width;
	minHeight = fw->hints.min_height;

	maxWidth = fw->hints.max_width;
	maxHeight =  fw->hints.max_height;

	if (maxWidth > fw->max_window_width - b.total_size.width)
	{
		maxWidth = fw->max_window_width - b.total_size.width;
	}
	if (maxHeight > fw->max_window_height - b.total_size.height)
	{
		maxHeight =
			fw->max_window_height - b.total_size.height;
	}

	baseWidth = fw->hints.base_width;
	baseHeight = fw->hints.base_height;

	xinc = fw->hints.width_inc;
	yinc = fw->hints.height_inc;

	/*
	 * First, clamp to min and max values
	 */
	if (dwidth < minWidth)
	{
		dwidth = minWidth;
	}
	if (dheight < minHeight)
	{
		dheight = minHeight;
	}
	if (dwidth > maxWidth)
	{
		dwidth = maxWidth;
	}
	if (dheight > maxHeight)
	{
		dheight = maxHeight;
	}

	/*
	 * Second, round to base + N * inc (up or down depending on resize type)
	 * if rounding up store amount
	 */
	if (!(flags & CS_ROUND_UP))
	{
		dwidth = (((dwidth - baseWidth) / xinc) * xinc) + baseWidth;
		dheight = (((dheight - baseHeight) / yinc) * yinc) + baseHeight;
	}
	else
	{
		roundUpX = dwidth;
		roundUpY = dheight;
		dwidth = (((dwidth - baseWidth + xinc - 1) / xinc) * xinc) +
			baseWidth;
		dheight = (((dheight - baseHeight + yinc - 1) / yinc) * yinc) +
			baseHeight;
		roundUpX = dwidth - roundUpX;
		roundUpY = dheight - roundUpY;
	}

	/*
	 * Step 2a: check we didn't move the edge off screen in interactive
	 * moves
	 */
	if ((flags & CS_ROUND_UP) && Event.type == MotionNotify)
	{
		if (xmotion > 0 && Event.xmotion.x_root < roundUpX)
		{
			dwidth -= xinc;
		}
		else if (xmotion < 0 &&
			 Event.xmotion.x_root >= Scr.MyDisplayWidth - roundUpX)
		{
			dwidth -= xinc;
		}
		if (ymotion > 0 && Event.xmotion.y_root < roundUpY)
		{
			dheight -= yinc;
		}
		else if (ymotion < 0 &&
			 Event.xmotion.y_root >= Scr.MyDisplayHeight - roundUpY)
		{
			dheight -= yinc;
		}
	}

	/*
	 * Step 2b: Check that we didn't violate min and max.
	 */
	if (dwidth < minWidth)
	{
		dwidth += xinc;
	}
	if (dheight < minHeight)
	{
		dheight += yinc;
	}
	if (dwidth > maxWidth)
	{
		dwidth -= xinc;
	}
	if (dheight > maxHeight)
	{
		dheight -= yinc;
	}

	/*
	 * Third, adjust for aspect ratio
	 */
#define maxAspectX fw->hints.max_aspect.x
#define maxAspectY fw->hints.max_aspect.y
#define minAspectX fw->hints.min_aspect.x
#define minAspectY fw->hints.min_aspect.y
	/*
	 * The math looks like this:
	 *
	 * minAspectX    dwidth     maxAspectX
	 * ---------- <= ------- <= ----------
	 * minAspectY    dheight    maxAspectY
	 *
	 * If that is multiplied out, then the width and height are
	 * invalid in the following situations:
	 *
	 * minAspectX * dheight > minAspectY * dwidth
	 * maxAspectX * dheight < maxAspectY * dwidth
	 *
	 */

	if (fw->hints.flags & PAspect)
	{

		if (fw->hints.flags & PBaseSize)
		{
			/*
			 * ICCCM 2 demands that aspect ratio should apply
			 * to width - base_width. To prevent funny results,
			 * we reset PBaseSize in GetWindowSizeHints, if
			 * base is not smaller than min.
			*/
			dwidth -= baseWidth;
			maxWidth -= baseWidth;
			minWidth -= baseWidth;
			dheight -= baseHeight;
			maxHeight -= baseHeight;
			minHeight -= baseHeight;
		}

		if ((minAspectX * dheight > minAspectY * dwidth) &&
		    (xmotion == 0))
		{
			/* Change width to match */
			delta = MAKEMULT(
				minAspectX * dheight / minAspectY - dwidth,
				xinc);
			if (dwidth + delta <= maxWidth)
			{
				dwidth += delta;
			}
		}
		if (minAspectX * dheight > minAspectY * dwidth)
		{
			delta = MAKEMULT(
				dheight - dwidth*minAspectY/minAspectX, yinc);
			if (dheight - delta >= minHeight)
			{
				dheight -= delta;
			}
			else
			{
				delta = MAKEMULT(
					minAspectX*dheight / minAspectY -
					dwidth, xinc);
				if (dwidth + delta <= maxWidth)
				{
					dwidth += delta;
				}
			}
		}

		if ((maxAspectX * dheight < maxAspectY * dwidth) &&
		    (ymotion == 0))
		{
			delta = MAKEMULT(
				dwidth * maxAspectY / maxAspectX - dheight,
				yinc);
			if (dheight + delta <= maxHeight)
			{
				dheight += delta;
			}
		}
		if ((maxAspectX * dheight < maxAspectY * dwidth))
		{
			delta = MAKEMULT(dwidth - maxAspectX*dheight/maxAspectY,
					 xinc);
			if (dwidth - delta >= minWidth)
			{
				dwidth -= delta;
			}
			else
			{
				delta = MAKEMULT(
					dwidth * maxAspectY / maxAspectX -
					dheight, yinc);
				if (dheight + delta <= maxHeight)
				{
					dheight += delta;
				}
			}
		}

		if (fw->hints.flags & PBaseSize)
		{
			dwidth += baseWidth;
			dheight += baseHeight;
		}
	}


	/*
	 * Fourth, account for border width and title height
	 */
	*widthp = dwidth + b.total_size.width;
	*heightp = dheight + b.total_size.height;
	if (IS_MAXIMIZED(fw) && (flags & CS_UPDATE_MAX_DEFECT))
	{
		/* update size defect for maximized window */
		fw->max_g_defect.width = old_w - *widthp;
		fw->max_g_defect.height = old_h - *heightp;
	}

	return;
}

/* This function does roughly the same as constrain_size, but takes into account
 * that the window shifts according to gravity if constrain_size actually
 * changes the width or height. The frame_g of the window is not changed. The
 * target geometry is expected to be in *rect and will be retured through rect.
 */
void gravity_constrain_size(
	int gravity, FvwmWindow *t, rectangle *rect, int flags)
{
	int new_width = rect->width;
	int new_height = rect->height;

	if (IS_MAXIMIZED(t) && (flags & CS_UPDATE_MAX_DEFECT))
	{
		gravity_resize(
			gravity, rect, t->max_g_defect.width,
			t->max_g_defect.height);
		t->max_g_defect.width = 0;
		t->max_g_defect.height = 0;
		new_width = rect->width;
		new_height = rect->height;
	}
	constrain_size(
		t, (unsigned int *)&new_width, (unsigned int *)&new_height, 0,
		0, flags);
	if (rect->width != new_width || rect->height != new_height)
	{
		gravity_resize(
			gravity, rect, new_width - rect->width,
			new_height - rect->height);
	}

	return;
}

/* returns the icon title geometry if it is visible */
Bool get_visible_icon_title_geometry(
	FvwmWindow *fw, rectangle *ret_g)
{
	if (HAS_NO_ICON_TITLE(fw) || IS_ICON_UNMAPPED(fw) ||
	    !IS_ICONIFIED(fw))
	{
		memset(ret_g, 0, sizeof(*ret_g));
		return False;
	}
	*ret_g = fw->icon_g.title_w_g;

	return True;
}

/* returns the icon title geometry if it the icon title window exists */
Bool get_icon_title_geometry(
	FvwmWindow *fw, rectangle *ret_g)
{
	if (HAS_NO_ICON_TITLE(fw))
	{
		memset(ret_g, 0, sizeof(*ret_g));
		return False;
	}
	*ret_g = fw->icon_g.title_w_g;

	return True;
}

/* returns the icon picture geometry if it is visible */
Bool get_visible_icon_picture_geometry(
	FvwmWindow *fw, rectangle *ret_g)
{
	if (fw->icon_g.picture_w_g.width == 0 ||
	    IS_ICON_UNMAPPED(fw) || !IS_ICONIFIED(fw))
	{
		memset(ret_g, 0, sizeof(*ret_g));
		return False;
	}
	*ret_g = fw->icon_g.picture_w_g;

	return True;
}

/* returns the icon picture geometry if it is exists */
Bool get_icon_picture_geometry(
	FvwmWindow *fw, rectangle *ret_g)
{
	if (fw->icon_g.picture_w_g.width == 0)
	{
		memset(ret_g, 0, sizeof(*ret_g));
		return False;
	}
	*ret_g = fw->icon_g.picture_w_g;

	return True;
}

/* returns the icon geometry (unexpanded title plus pixmap) if it is visible */
Bool get_visible_icon_geometry(
	FvwmWindow *fw, rectangle *ret_g)
{
	if (IS_ICON_UNMAPPED(fw) || !IS_ICONIFIED(fw))
	{
		memset(ret_g, 0, sizeof(*ret_g));
		return False;
	}
	if (fw->icon_g.picture_w_g.width > 0)
	{
		*ret_g = fw->icon_g.picture_w_g;
		if (!HAS_NO_ICON_TITLE(fw))
		{
			ret_g->height += fw->icon_g.title_w_g.height;
		}
	}
	else if (!HAS_NO_ICON_TITLE(fw))
	{
		*ret_g = fw->icon_g.title_w_g;
	}
	else
	{
		memset(ret_g, 0, sizeof(*ret_g));
		return False;
	}

	return True;
}

/* returns the icon geometry (unexpanded title plus pixmap) if it exists */
Bool get_icon_geometry(
	FvwmWindow *fw, rectangle *ret_g)
{
	if (fw->icon_g.picture_w_g.width > 0)
	{
		*ret_g = fw->icon_g.picture_w_g;
		if (!HAS_NO_ICON_TITLE(fw))
		{
			ret_g->height += fw->icon_g.title_w_g.height;
		}
	}
	else if (!HAS_NO_ICON_TITLE(fw))
	{
		*ret_g = fw->icon_g.title_w_g;
	}
	else
	{
		memset(ret_g, 0, sizeof(*ret_g));
		return False;
	}

	return True;
}

/* Returns the visible geometry of a window or icon.  This can be used to test
 * if this region overlaps other windows. */
Bool get_visible_window_or_icon_geometry(
	FvwmWindow *fw, rectangle *ret_g)
{
	if (IS_ICONIFIED(fw))
	{
		return get_visible_icon_geometry(fw, ret_g);
	}
	*ret_g = fw->frame_g;

	return True;
}

void move_icon_to_position(
	FvwmWindow *fw)
{
	if (fw->icon_g.picture_w_g.width > 0)
	{
		XMoveWindow(
			dpy, FW_W_ICON_PIXMAP(fw),
			fw->icon_g.picture_w_g.x,
			fw->icon_g.picture_w_g.y);
	}
	if (!HAS_NO_ICON_TITLE(fw))
	{
		XMoveWindow(
			dpy, FW_W_ICON_TITLE(fw),
			fw->icon_g.title_w_g.x,
			fw->icon_g.title_w_g.y);
	}

	return;
}

void broadcast_icon_geometry(
	FvwmWindow *fw, Bool do_force)
{
	rectangle g;
	Bool rc;

	rc = get_visible_icon_geometry(fw, &g);
	if (rc == True && (!IS_ICON_UNMAPPED(fw) || do_force == True))
	{
		BroadcastPacket(
			M_ICON_LOCATION, 7, FW_W(fw), FW_W_FRAME(fw),
			(unsigned long)fw,
			g.x, g.y, g.width, g.height);
	}

	return;
}

void modify_icon_position(
	FvwmWindow *fw, int dx, int dy)
{
	if (fw->icon_g.picture_w_g.width > 0)
	{
		fw->icon_g.picture_w_g.x += dx;
		fw->icon_g.picture_w_g.y += dy;
	}
	if (!HAS_NO_ICON_TITLE(fw))
	{
		fw->icon_g.title_w_g.x += dx;
		fw->icon_g.title_w_g.y += dy;
	}

	return;
}

/* set the icon position to the specified value. take care of the actual icon
 * layout */
void set_icon_position(
	FvwmWindow *fw, int x, int y)
{
	if (fw->icon_g.picture_w_g.width > 0)
	{
		fw->icon_g.picture_w_g.x = x;
		fw->icon_g.picture_w_g.y = y;
	}
	else
	{
		fw->icon_g.picture_w_g.x = 0;
		fw->icon_g.picture_w_g.y = 0;
	}
	if (!HAS_NO_ICON_TITLE(fw))
	{
		fw->icon_g.title_w_g.x = x;
		fw->icon_g.title_w_g.y = y;
	}
	else
	{
		fw->icon_g.title_w_g.x = 0;
		fw->icon_g.title_w_g.y = 0;
	}
	if (fw->icon_g.picture_w_g.width > 0 &&
	    !HAS_NO_ICON_TITLE(fw))
	{
		fw->icon_g.title_w_g.x -=
			(fw->icon_g.title_w_g.width -
			 fw->icon_g.picture_w_g.width) / 2;
		fw->icon_g.title_w_g.y +=
			fw->icon_g.picture_w_g.height;
	}

	return;
}

void set_icon_picture_size(
	FvwmWindow *fw, int w, int h)
{
	if (fw->icon_g.picture_w_g.width > 0)
	{
		fw->icon_g.picture_w_g.width = w;
		fw->icon_g.picture_w_g.height = h;
	}
	else
	{
		fw->icon_g.picture_w_g.width = 0;
		fw->icon_g.picture_w_g.height = 0;
	}

	return;
}

void resize_icon_title_height(FvwmWindow *fw, int dh)
{
	if (!HAS_NO_ICON_TITLE(fw))
	{
		fw->icon_g.title_w_g.height += dh;
	}

	return;
}

/* ---------------------------- builtin commands ---------------------------- */

