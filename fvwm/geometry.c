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

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "misc.h"
#include "screen.h"
#include "geometry.h"
#include "module_interface.h"
#include "borders.h"
#include "icons.h"
#include "add_window.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

/* ---------------------------- interface functions ------------------------ */

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

void get_absolute_geometry(rectangle *abs_g, rectangle *rel_g)
{
	abs_g->x = rel_g->x + Scr.Vx;
	abs_g->y = rel_g->y + Scr.Vy;
	abs_g->width = rel_g->width;
	abs_g->height = rel_g->height;

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
	w = (ret_g->width > 0) ? ret_g->width : fw->g.frame.width;
	h = (ret_g->height > 0) ? ret_g->height : fw->g.frame.height;
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
		ret_g->x += fw->g.frame.x;
		ret_g->y += fw->g.frame.y;

		return True;

	}
	bnum = get_button_number(context);
	if (bnum < 0 || FW_W_BUTTON(fw, bnum) == None)
	{
		return False;
	}
	if (XGetGeometry(
		dpy, FW_W_BUTTON(fw, bnum), &JunkRoot, &ret_g->x, &ret_g->y,
		(unsigned int*)&ret_g->width, (unsigned int*)&ret_g->height,
		(unsigned int*)&JunkBW, (unsigned int*)&JunkDepth) == 0)
	{
		return False;
	}
	XTranslateCoordinates(
		dpy, FW_W_FRAME(fw), Scr.Root, ret_g->x, ret_g->y, &ret_g->x,
		&ret_g->y, &JunkChild);

	return True;
}

void get_title_font_size_and_offset(
	FvwmWindow *fw, direction_t title_dir,
	Bool is_left_title_rotated_cw, Bool is_right_title_rotated_cw,
	Bool is_top_title_rotated, Bool is_bottom_title_rotated,
	int *size, int *offset)
{
	int decor_size;
	int extra_size;
	int font_size;
	int min_offset;
	Bool is_rotated_cw, is_rotated;
	rotation_t draw_rotation;

	/* adjust font offset according to height specified in title style */
	decor_size = fw->decor->title_height;
	font_size = fw->title_font->height + EXTRA_TITLE_FONT_HEIGHT;
	switch (title_dir)
	{
	case DIR_W:
	case DIR_E:
		is_rotated_cw = (title_dir == DIR_W) ?
			is_left_title_rotated_cw : is_right_title_rotated_cw;
		if (is_rotated_cw)
		{
			fw->title_text_rotation = ROTATION_90;
		}
		else
		{
			fw->title_text_rotation = ROTATION_270;
		}
		break;
	case DIR_N:
	case DIR_S:
	default:
		is_rotated = (title_dir == DIR_N) ?
			is_top_title_rotated : is_bottom_title_rotated;
		if (is_rotated)
		{
			fw->title_text_rotation = ROTATION_180;
		}
		else
		{
			fw->title_text_rotation = ROTATION_0;
		}
		break;
	}
	if (USE_TITLE_DECOR_ROTATION(fw))
	{
		draw_rotation = ROTATION_0;
	}
	else
	{
		draw_rotation = fw->title_text_rotation;
	}
	min_offset =  FlocaleGetMinOffset(
		fw->title_font, draw_rotation);
	extra_size = (decor_size > 0) ? decor_size - font_size : 0;
	*offset = min_offset;
	if (fw->decor->min_title_height > 0 &&
	    font_size + extra_size < fw->decor->min_title_height)
	{
		extra_size = fw->decor->min_title_height - font_size;
	}
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
		ret_g->x = fw->g.frame.x;
		ret_g->y = fw->g.frame.y;
		break;
	case DIR_S:
		ret_g->x = fw->g.frame.x;
		ret_g->y = fw->g.frame.y + fw->g.frame.height -
			ret_g->height;
		break;
	case DIR_E:
		ret_g->x = fw->g.frame.x + fw->g.frame.width -
			ret_g->width;
		ret_g->y = fw->g.frame.y;
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
	int d;

	switch (SHADED_DIR(fw))
	{
	case DIR_SW:
	case DIR_SE:
	case DIR_NW:
	case DIR_NE:
		get_window_borders_no_title(fw, &b);
		break;
	default:
		get_window_borders(fw, &b);
		break;
	}
	*small_g = *big_g;
	d = 0;
	switch (SHADED_DIR(fw))
	{
	case DIR_S:
	case DIR_SW:
	case DIR_SE:
		small_g->y = big_g->y + big_height - b.total_size.height;
		d = 1;
		/* fall through */
	case DIR_N:
	case DIR_NW:
	case DIR_NE:
		small_g->height = b.total_size.height;
		if (small_g->height == 0)
		{
			small_g->height = 1;
			small_g->y -= d;
		}
		break;
	default:
		break;
	}
	d = 0;
	switch (SHADED_DIR(fw))
	{
	case DIR_E:
	case DIR_NE:
	case DIR_SE:
		small_g->x = big_g->x + big_width - b.total_size.width;
		d = 1;
		/* fall through */
	case DIR_W:
	case DIR_NW:
	case DIR_SW:
		small_g->width = b.total_size.width;
		if (small_g->width == 0)
		{
			small_g->width = 1;
			small_g->x -= d;
		}
		break;
	default:
		break;
	}

	return;
}

void get_shaded_geometry_with_dir(
	FvwmWindow *fw, rectangle *small_g, rectangle *big_g,
	direction_t shade_dir)
{
	direction_t old_shade_dir;

	old_shade_dir = SHADED_DIR(fw);
	SET_SHADED_DIR(fw, shade_dir);
	get_shaded_geometry(fw, small_g, big_g);
	SET_SHADED_DIR(fw, old_shade_dir);

	return;
}

void get_unshaded_geometry(
	FvwmWindow *fw, rectangle *ret_g)
{
	if (IS_SHADED(fw))
	{
		if (IS_MAXIMIZED(fw))
		{
			*ret_g = fw->g.max;
		}
		else
		{
			*ret_g = fw->g.normal;
		}
		get_relative_geometry(ret_g, ret_g);
	}
	else
	{
		*ret_g = fw->g.frame;
	}

	return;
}

void get_shaded_client_window_pos(
	FvwmWindow *fw, rectangle *ret_g)
{
	rectangle big_g;
	size_borders b;

	get_window_borders(fw, &b);
	big_g = (IS_MAXIMIZED(fw)) ? fw->g.max : fw->g.normal;
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
	const FvwmWindow *fw, size_borders *borders)
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
	const FvwmWindow *fw, size_borders *borders)
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
	FvwmWindow *fw, int used_width)
{
	if (used_width <= 0)
	{
		fw->boundary_width = 0;
		fw->unshaped_boundary_width = 0;
	}
	else
	{
		fw->unshaped_boundary_width = used_width;
		fw->boundary_width = (fw->wShaped) ? 0 : used_width;
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

/* update the frame_g according to the window's g.normal or g.max and shaded
 * state */
void update_relative_geometry(FvwmWindow *fw)
{
	get_relative_geometry(
		&fw->g.frame,
		(IS_MAXIMIZED(fw)) ? &fw->g.max : &fw->g.normal);
	if (IS_SHADED(fw))
	{
		get_shaded_geometry(
			fw, &fw->g.frame, &fw->g.frame);
	}

	return;
}

/* update the g.normal or g.max according to the window's current position */
void update_absolute_geometry(FvwmWindow *fw)
{
	rectangle *dest_g;
	rectangle frame_g;

	/* store orig values in absolute coords */
	dest_g = (IS_MAXIMIZED(fw)) ? &fw->g.max : &fw->g.normal;
	frame_g = *dest_g;
	dest_g->x = fw->g.frame.x + Scr.Vx;
	dest_g->y = fw->g.frame.y + Scr.Vy;
	dest_g->width = fw->g.frame.width;
	dest_g->height = fw->g.frame.height;
	if (IS_SHADED(fw))
	{
		switch (SHADED_DIR(fw))
		{
		case DIR_SW:
		case DIR_S:
		case DIR_SE:
			dest_g->y += fw->g.frame.height - frame_g.height;
			/* fall through */
		case DIR_NW:
		case DIR_N:
		case DIR_NE:
			dest_g->height = frame_g.height;
			break;
		}
		switch (SHADED_DIR(fw))
		{
		case DIR_NE:
		case DIR_E:
		case DIR_SE:
			dest_g->x += fw->g.frame.width - frame_g.width;
			/* fall through */
		case DIR_NW:
		case DIR_W:
		case DIR_SW:
			dest_g->width = frame_g.width;
			break;
		}
	}

	return;
}

/* make sure a maximized window and it's normal version are never a page or
 * more apart. */
void maximize_adjust_offset(FvwmWindow *fw)
{
	int off_x;
	int off_y;
	int dh;
	int dw;

	if (!IS_MAXIMIZED(fw))
	{
		/* otherwise we might corrupt the g.normal */
		return;
	}
	off_x = fw->g.normal.x - fw->g.max.x - fw->g.max_offset.x;
	off_y = fw->g.normal.y - fw->g.max.y - fw->g.max_offset.y;
	dw = Scr.MyDisplayWidth;
	dh = Scr.MyDisplayHeight;
	if (off_x >= dw)
	{
		fw->g.normal.x -= (off_x / dw) * dw;
	}
	else if (off_x <= -dw)
	{
		fw->g.normal.x += (-off_x / dw) * dw;
	}
	if (off_y >= dh)
	{
		fw->g.normal.y -= (off_y / dh) * dh;
	}
	else if (off_y <= -dh)
	{
		fw->g.normal.y += (-off_y / dh) * dh;
	}

	return;
}

#define MAKEMULT(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
static void __cs_handle_aspect_ratio(
	size_rect *ret_s, FvwmWindow *fw, size_rect s, const size_rect base,
	const size_rect inc, size_rect min, size_rect max, int xmotion,
	int ymotion, int flags)
{
	volatile double odefect;
	volatile double defect;
	volatile double rmax;
	volatile double rmin;
	volatile int delta;
	volatile int ow;
	volatile int oh;

	if (fw->hints.flags & PBaseSize)
	{
		/*
		 * ICCCM 2 demands that aspect ratio should apply to width -
		 * base_width. To prevent funny results, we reset PBaseSize in
		 * GetWindowSizeHints, if base is not smaller than min.
		 */
		s.width -= base.width;
		max.width -= base.width;
		min.width -= base.width;
		s.height -= base.height;
		max.height -= base.height;
		min.height -= base.height;
	}
	rmin = (double)fw->hints.min_aspect.x / (double)fw->hints.min_aspect.y;
	rmax = (double)fw->hints.max_aspect.x / (double)fw->hints.max_aspect.y;
	do
	{
		double r;

		r = (double)s.width / (double)s.height;
		ow = s.width;
		oh = s.height;
		odefect = 0;
		if (r < rmin)
		{
			odefect = rmin - r;
		}
		else if (r > rmax)
		{
			odefect = r - rmax;
		}
		if (r < rmin && (flags & CS_ROUND_UP) && xmotion == 0)
		{
			/* change width to match */
			delta = MAKEMULT(s.height * rmin - s.width, inc.width);
			if (s.width + delta <= max.width)
			{
				s.width += delta;
			}
			r = (double)s.width / (double)s.height;
		}
		if (r < rmin)
		{
			/* change height to match */
			delta = MAKEMULT(
				s.height - s.width / rmin, inc.height);
			if (s.height - delta >= min.height)
			{
				s.height -= delta;
			}
			else
			{
				delta = MAKEMULT(
					s.height * rmin - s.width, inc.width);
				if (s.width + delta <= max.width)
				{
					s.width += delta;
				}
			}
			r = (double)s.width / (double)s.height;
		}

		if (r > rmax && (flags & CS_ROUND_UP) && ymotion == 0)
		{
			/* change height to match */
			delta = MAKEMULT(s.width /rmax - s.height, inc.height);
			if (s.height + delta <= max.height)
			{
				s.height += delta;
			}
			r = (double)s.width / (double)s.height;
		}
		if (r > rmax)
		{
			/* change width to match */
			delta = MAKEMULT(s.width - s.height * rmax, inc.width);
			if (s.width - delta >= min.width)
			{
				s.width -= delta;
			}
			else
			{
				delta = MAKEMULT(
					s.width / rmax - s.height, inc.height);
				if (s.height + delta <= max.height)
				{
					s.height += delta;
				}
			}
			r = (double)s.width / (double)s.height;
		}
		defect = 0;
		if (r < rmin)
		{
			defect = rmin - r;
		}
		else if (r > rmax)
		{
			defect = r - rmax;
		}
	} while (odefect > defect);
	if (fw->hints.flags & PBaseSize)
	{
		ow += base.width;
		oh += base.height;
	}
	ret_s->width = ow;
	ret_s->height = oh;

	return;
}

/*
 *
 *  Procedure:
 *      constrain_size - adjust the given width and height to account for the
 *              constraints imposed by size hints
 */
void constrain_size(
	FvwmWindow *fw, const XEvent *e, int *widthp, int *heightp,
	int xmotion, int ymotion, int flags)
{
	size_rect min;
	size_rect max;
	size_rect inc;
	size_rect base;
	size_rect round_up;
	size_rect d;
	size_rect old;
	size_borders b;

	if (DO_DISABLE_CONSTRAIN_SIZE_FULLSCREEN(fw) == 1)
	{
		return;
	}
	if (HAS_NEW_WM_NORMAL_HINTS(fw))
	{
		/* get the latest size hints */
		XSync(dpy, 0);
		GetWindowSizeHints(fw);
		SET_HAS_NEW_WM_NORMAL_HINTS(fw, 0);
	}
	if (IS_MAXIMIZED(fw) && (flags & CS_UPDATE_MAX_DEFECT))
	{
		*widthp += fw->g.max_defect.width;
		*heightp += fw->g.max_defect.height;
	}
	/* gcc 4.1.1 warns about these not being initialized at the end,
	 * but the conditions for the use are the same...*/
	old.width = *widthp;
	old.height = *heightp;

	d.width = *widthp;
	d.height = *heightp;
	get_window_borders(fw, &b);
	d.width -= b.total_size.width;
	d.height -= b.total_size.height;

	min.width = fw->hints.min_width;
	min.height = fw->hints.min_height;
	if (min.width < fw->min_window_width - b.total_size.width)
	{
		min.width = fw->min_window_width - b.total_size.width;
	}
	if (min.height < fw->min_window_height - b.total_size.height)
	{
		min.height =
			fw->min_window_height - b.total_size.height;
	}

	max.width = fw->hints.max_width;
	max.height =  fw->hints.max_height;
	if (max.width > fw->max_window_width - b.total_size.width)
	{
		max.width = fw->max_window_width - b.total_size.width;
	}
	if (max.height > fw->max_window_height - b.total_size.height)
	{
		max.height =
			fw->max_window_height - b.total_size.height;
	}

	if (min.width > max.width)
	{
		min.width = max.width;
	}
	if (min.height > max.height)
	{
		min.height = max.height;
	}

	base.width = fw->hints.base_width;
	base.height = fw->hints.base_height;

	inc.width = fw->hints.width_inc;
	inc.height = fw->hints.height_inc;

	/*
	 * First, clamp to min and max values
	 */
	if (d.width < min.width)
	{
		d.width = min.width;
	}
	if (d.height < min.height)
	{
		d.height = min.height;
	}
	if (d.width > max.width)
	{
		d.width = max.width;
	}
	if (d.height > max.height)
	{
		d.height = max.height;
	}

	/*
	 * Second, round to base + N * inc (up or down depending on resize
	 * type) if rounding up store amount
	 */
	if (!(flags & CS_ROUND_UP))
	{
		d.width = ((d.width - base.width) / inc.width) *
			inc.width + base.width;
		d.height = ((d.height - base.height) / inc.height) *
			inc.height + base.height;
	}
	else
	{
		round_up.width = d.width;
		round_up.height = d.height;
		d.width = ((d.width - base.width + inc.width - 1) /
			   inc.width) * inc.width + base.width;
		d.height = ((d.height - base.height + inc.height - 1) /
			    inc.height) * inc.height + base.height;
		round_up.width = d.width - round_up.width;
		round_up.height = d.height - round_up.height;
	}

	/*
	 * Step 2a: check we didn't move the edge off screen in interactive
	 * moves
	 */
	if ((flags & CS_ROUND_UP) && e != NULL && e->type == MotionNotify)
	{
		if (xmotion > 0 && e->xmotion.x_root < round_up.width)
		{
			d.width -= inc.width;
		}
		else if (
			xmotion < 0 && e->xmotion.x_root >=
			Scr.MyDisplayWidth - round_up.width)
		{
			d.width -= inc.width;
		}
		if (ymotion > 0 && e->xmotion.y_root < round_up.height)
		{
			d.height -= inc.height;
		}
		else if (
			ymotion < 0 && e->xmotion.y_root >=
			Scr.MyDisplayHeight - round_up.height)
		{
			d.height -= inc.height;
		}
	}

	/*
	 * Step 2b: Check that we didn't violate min and max.
	 */
	if (d.width < min.width)
	{
		d.width += inc.width;
	}
	if (d.height < min.height)
	{
		d.height += inc.height;
	}
	if (d.width > max.width)
	{
		d.width -= inc.width;
	}
	if (d.height > max.height)
	{
		d.height -= inc.height;
	}

	/*
	 * Third, adjust for aspect ratio
	 */
	if (fw->hints.flags & PAspect)
	{
		__cs_handle_aspect_ratio(
			&d, fw, d, base, inc, min, max, xmotion, ymotion,
			flags);
	}

	/*
	 * Fourth, account for border width and title height
	 */
	*widthp = d.width + b.total_size.width;
	*heightp = d.height + b.total_size.height;
	if (IS_MAXIMIZED(fw) && (flags & CS_UPDATE_MAX_DEFECT))
	{
		/* update size defect for maximized window */
		fw->g.max_defect.width = old.width - *widthp;
		fw->g.max_defect.height = old.height - *heightp;
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
			gravity, rect, t->g.max_defect.width,
			t->g.max_defect.height);
		t->g.max_defect.width = 0;
		t->g.max_defect.height = 0;
		new_width = rect->width;
		new_height = rect->height;
	}
	constrain_size(
		t, NULL, &new_width, &new_height, 0, 0, flags);
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
void get_icon_geometry(
	FvwmWindow *fw, rectangle *ret_g)
{
	/* valid geometry? */
	if (fw->icon_g.picture_w_g.width > 0)
	{
		*ret_g = fw->icon_g.picture_w_g;
		if (!HAS_NO_ICON_TITLE(fw))
		{
			ret_g->height += fw->icon_g.title_w_g.height;
		}
	}
	else if (fw->icon_g.title_w_g.width > 0)
	{
		*ret_g = fw->icon_g.title_w_g;
	}
	/* valid position? */
	else if (fw->icon_g.picture_w_g.x != 0 || fw->icon_g.picture_w_g.y != 0)
	{
		*ret_g = fw->icon_g.picture_w_g;
	}
	else if (fw->icon_g.title_w_g.x != 0 || fw->icon_g.title_w_g.y != 0)
	{
		*ret_g = fw->icon_g.title_w_g;
	}
	else
	{
		memset(ret_g, 0, sizeof(*ret_g));
	}

	return;
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
	*ret_g = fw->g.frame;

	return True;
}

void move_icon_to_position(
	FvwmWindow *fw)
{
	Bool draw_picture_w = False;
	Bool draw_title_w = False;

	if (fw->icon_g.picture_w_g.width > 0)
	{
		int cs;

		if (Scr.Hilite == fw)
		{
			cs = fw->cs_hi;
		}
		else
		{
			cs = fw->cs;
		}
		XMoveWindow(
			dpy, FW_W_ICON_PIXMAP(fw),
			fw->icon_g.picture_w_g.x,
			fw->icon_g.picture_w_g.y);
		if (fw->icon_alphaPixmap ||
		    (cs >= 0 && Colorset[cs].icon_alpha_percent < 100) ||
		    CSET_IS_TRANSPARENT(fw->icon_background_cs) ||
		    (!IS_ICON_SHAPED(fw) && fw->icon_background_padding > 0))
		{
			draw_picture_w = True;
		}
	}
	if (!HAS_NO_ICON_TITLE(fw))
	{
		int cs;
		rectangle dummy;

		if (Scr.Hilite == fw)
		{
			cs = fw->icon_title_cs_hi;
		}
		else
		{
			cs = fw->icon_title_cs;
		}
		XMoveWindow(
			dpy, FW_W_ICON_TITLE(fw),
			fw->icon_g.title_w_g.x,
			fw->icon_g.title_w_g.y);
		if (CSET_IS_TRANSPARENT(cs) &&
		    !get_visible_icon_picture_geometry(fw, &dummy) &&
		    get_visible_icon_title_geometry(fw, &dummy))
		{
			draw_title_w = True;
		}
	}

	if (draw_title_w || draw_picture_w)
	{
		DrawIconWindow(
			fw, draw_title_w, draw_picture_w, False, draw_picture_w,
			NULL);
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
			M_ICON_LOCATION, 7, (long)FW_W(fw),
			(long)FW_W_FRAME(fw), (unsigned long)fw,
			(long)g.x, (long)g.y, (long)g.width, (long)g.height);
	}

	return;
}

void modify_icon_position(
	FvwmWindow *fw, int dx, int dy)
{
	if (fw->icon_g.picture_w_g.width > 0 || HAS_NO_ICON_TITLE(fw))
	{
		/* picture position is also valid if there is neither a picture
		 * nor a title */
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
	else if (fw->icon_g.picture_w_g.width <= 0 && HAS_NO_ICON_TITLE(fw))
	{
		/* In case there is no icon, fake the icon position so the
		 * modules know where its window was iconified. */
		fw->icon_g.picture_w_g.x = x;
		fw->icon_g.picture_w_g.y = y;
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

void get_page_offset_rectangle(
	int *ret_page_x, int *ret_page_y, rectangle *r)
{
	int xoff = Scr.Vx % Scr.MyDisplayWidth;
	int yoff = Scr.Vy % Scr.MyDisplayHeight;

	/* maximize on the page where the center of the window is */
	*ret_page_x = truncate_to_multiple(
		r->x + r->width / 2 + xoff, Scr.MyDisplayWidth) - xoff;
	*ret_page_y = truncate_to_multiple(
		r->y + r->height / 2 + yoff, Scr.MyDisplayHeight) - yoff;

	return;
}

void get_page_offset(
	int *ret_page_x, int *ret_page_y, FvwmWindow *fw)
{
	rectangle r;

	r.x = fw->g.frame.x;
	r.y = fw->g.frame.y;
	r.width = fw->g.frame.width;
	r.height = fw->g.frame.height;
	get_page_offset_rectangle(ret_page_x, ret_page_y, &r);

	return;
}

void get_page_offset_check_visible(
	int *ret_page_x, int *ret_page_y, FvwmWindow *fw)
{
	if (IsRectangleOnThisPage(&fw->g.frame, fw->Desk))
	{
		/* maximize on visible page if any part of the window is
		 * visible */
		*ret_page_x = 0;
		*ret_page_y = 0;
	}
	else
	{
		get_page_offset(ret_page_x, ret_page_y, fw);
	}

	return;
}

/* ---------------------------- builtin commands --------------------------- */
