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

/****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/Flocale.h"
#include <libs/gravity.h>
#include "libs/fvwmrect.h"
#include "libs/gravity.h"
#include "fvwm.h"
#include "externs.h"
#include "events.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "defaults.h"
#include "geometry.h"
#include "module_interface.h"
#include "frame.h"
#include "gnome.h"
#include "focus.h"
#include "ewmh.h"
#include "borders.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

typedef enum
{
	/* actions that are only executed in the first step, but not if there
	 * is only one step */
	RA_FIRST_RAISE_PARENT,
	RA_FIRST_SETUP_HIDDEN,
	RA_FIRST_NOT_LAST_END,
	/* actions that are only executed in the first step */
	RA_FIRST_END,
	/* actions that are only executed in every step */
	RA_X_RESIZE_TITLE,
	RA_Y_RESIZE_TITLE,
	RA_X_RESIZE_DECORW,
	RA_Y_RESIZE_DECORW,
	RA_X_RESIZE_CLIENT,
	RA_Y_RESIZE_CLIENT,
	RA_X_RESIZE_PARENT,
	RA_Y_RESIZE_PARENT,
	RA_RESIZE_DECORW,
	RA_RESIZE_CLIENT,
	RA_RESIZE_PARENT,
	RA_MOVERESIZE_FRAME,
	/* actions that are only executed in the last step */
	RA_LAST_BEGIN,
	RA_LAST_RESTORE_FOCUS,
	RA_LAST_LOWER_PARENT,
	RA_LAST_SETUP_HIDDEN,
	/* actions that are only executed in the last step, but not if there
	 * is only one step */
	RA_LAST_NOT_FIRST_BEGIN,
	RA_DONE
} resize_action_type;

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

static void print_g(char *text, rectangle *g)
{
	if (g == NULL)
	{
		fprintf(stderr, "%s: (null)", (text == NULL) ? "" : text);
	}
	else
	{
		fprintf(stderr, "%s: %4d %4d %4dx%4d (%4d - %4d %4d - %4d)\n",
			(text == NULL) ? "" : text,
			g->x, g->y, g->width, g->height,
			g->x, g->x + g->width - 1, g->y, g->y + g->height - 1);
	}
}

static Bool is_hidden(FvwmWindow *tmp_win, rectangle *g)
{
	size_borders b;

	get_window_borders(tmp_win, &b);
	if (g->width <= b.total_size.width || g->height <= b.total_size.height)
	{
		return True;
	}

	return False;
}

static void combine_decor_gravities(
	decor_gravities_type *ret_grav,
	decor_gravities_type *grav_x,
	decor_gravities_type *grav_y)
{
	ret_grav->decor_grav =
		gravity_combine_xy_grav(grav_x->decor_grav,
					grav_y->decor_grav);
	ret_grav->title_grav =
		gravity_combine_xy_grav(grav_x->title_grav,
					grav_y->title_grav);
	ret_grav->lbutton_grav =
		gravity_combine_xy_grav(grav_x->lbutton_grav,
					grav_y->lbutton_grav);
	ret_grav->rbutton_grav =
		gravity_combine_xy_grav(grav_x->rbutton_grav,
					grav_y->rbutton_grav);
	ret_grav->parent_grav =
		gravity_combine_xy_grav(grav_x->parent_grav,
					grav_y->parent_grav);
	ret_grav->client_grav =
		gravity_combine_xy_grav(grav_x->client_grav,
					grav_y->client_grav);

	return;
}

static void get_resize_decor_gravities_one_axis(
	decor_gravities_type *ret_grav,
	direction_type title_dir, resize_mode_type axis_mode,
	direction_type neg_dir, direction_type pos_dir)
{
	int title_grav;
	int neg_grav;
	int pos_grav;

	title_grav = gravity_dir_to_grav(title_dir);
	neg_grav = gravity_dir_to_grav(neg_dir);
	pos_grav = gravity_dir_to_grav(pos_dir);
	if (title_dir != DIR_NONE)
	{
		ret_grav->decor_grav = title_grav;
		ret_grav->lbutton_grav = title_grav;
		ret_grav->title_grav = title_grav;
		ret_grav->rbutton_grav = title_grav;
	}
	else
	{
		ret_grav->decor_grav = neg_grav;
		ret_grav->lbutton_grav = neg_grav;
		ret_grav->title_grav = neg_grav;
		ret_grav->rbutton_grav = pos_grav;
	}
	ret_grav->parent_grav = neg_grav;
	switch (axis_mode)
	{
	case RESIZE_MODE_SCROLL:
		ret_grav->client_grav = pos_grav;
		break;
	case RESIZE_MODE_SHRINK:
	case RESIZE_MODE_RESIZE:
	default:
		ret_grav->client_grav = neg_grav;
		break;
	}

	return;
}

static void execute_one_resize_action(
	FvwmWindow *tmp_win, resize_action_type action, rectangle *new_g,
	rectangle *old_g, rectangle *client_g, size_borders *b)
{
	switch (action)
	{
	case RA_FIRST_RAISE_PARENT:
		XRaiseWindow(dpy, tmp_win->Parent);
		break;
	case RA_X_RESIZE_TITLE:
		if (!HAS_VERTICAL_TITLE(tmp_win))
		{
			XResizeWindow(
				dpy, tmp_win->title_w,
				new_g->width - b->total_size.width,
				tmp_win->title_thickness);
		}
		break;
	case RA_Y_RESIZE_TITLE:
		if (HAS_VERTICAL_TITLE(tmp_win))
		{
			XResizeWindow(
				dpy, tmp_win->title_w, tmp_win->title_thickness,
				new_g->height - b->total_size.height);
		}
		break;
	case RA_X_RESIZE_DECORW:
		XResizeWindow(
			dpy, tmp_win->decor_w, new_g->width, old_g->height);
		break;
	case RA_Y_RESIZE_DECORW:
		XResizeWindow(
			dpy, tmp_win->decor_w, old_g->width, new_g->height);
		break;
	case RA_RESIZE_DECORW:
		XResizeWindow(
			dpy, tmp_win->decor_w, new_g->width, new_g->height);
		break;
	case RA_X_RESIZE_CLIENT:
		XResizeWindow(
			dpy, tmp_win->w,
			max(new_g->width - b->total_size.width, 1),
			max(old_g->height - b->total_size.height, 1));
		XFlush(dpy);
		break;
	case RA_Y_RESIZE_CLIENT:
		XResizeWindow(
			dpy, tmp_win->w,
			max(old_g->width - b->total_size.width, 1),
			max(new_g->height - b->total_size.height, 1));
		XFlush(dpy);
		break;
	case RA_RESIZE_CLIENT:
		XResizeWindow(
			dpy, tmp_win->w,
			max(new_g->width - b->total_size.width, 1),
			max(new_g->height - b->total_size.height, 1));
		XFlush(dpy);
		break;
	case RA_X_RESIZE_PARENT:
		XResizeWindow(
			dpy, tmp_win->Parent,
			max(new_g->width - b->total_size.width, 1),
			max(old_g->height - b->total_size.height, 1));
		break;
	case RA_Y_RESIZE_PARENT:
		XResizeWindow(
			dpy, tmp_win->Parent,
			max(old_g->width - b->total_size.width, 1),
			max(new_g->height - b->total_size.height, 1));
		break;
	case RA_RESIZE_PARENT:
		XResizeWindow(
			dpy, tmp_win->Parent,
			max(new_g->width - b->total_size.width, 1),
			max(new_g->height - b->total_size.height, 1));
		break;
	case RA_MOVERESIZE_FRAME:
		XMoveResizeWindow(
			dpy, tmp_win->frame, new_g->x, new_g->y, new_g->width,
			new_g->height);
		*old_g = *new_g;
		break;
	case RA_LAST_RESTORE_FOCUS:
		FOCUS_SET(tmp_win->w);
		break;
	case RA_LAST_LOWER_PARENT:
		XLowerWindow(dpy, tmp_win->Parent);
		break;
	case RA_FIRST_SETUP_HIDDEN:
	case RA_LAST_SETUP_HIDDEN:
#if 0
		tmp_g = (IS_MAXIMIZED(tmp_win)) ?
			tmp_win->max_g : tmp_win->normal_g;
		tmp_g.width -= b->total_size.width;
		tmp_g.height -= b->total_size.height;

		XMoveResizeWindow(
			dpy, tmp_win->Parent, ?, ?, client_g->width,
			client_g->height);
		XMoveResizeWindow(
			dpy, tmp_win->w, 0, 0, client_g->width,
			client_g->height);
#endif
		/*!!!*/
		break;
	default:
		/* should never happen */
		break;
	}

	return;
}

static void execute_resize_actions(
	FvwmWindow *tmp_win, resize_action_type *action, rectangle *new_g,
	rectangle *old_g, rectangle *client_g, Bool is_first_step,
	Bool is_last_step)
{
	int i;
	size_borders b;
	rectangle old_g2;

	get_window_borders(tmp_win, &b);
	old_g2 = *old_g;
	for (i = 0; action[i] != RA_DONE; i++)
	{
		if (action[i] < RA_FIRST_END && !is_first_step)
		{
			continue;
		}
		else if (action[i] < RA_FIRST_NOT_LAST_END &&
			 (!is_first_step || is_last_step))
		{
			continue;
		}
		else if (action[i] > RA_LAST_BEGIN && !is_last_step)
		{
			continue;
		}
		else if (action[i] > RA_LAST_NOT_FIRST_BEGIN &&
			 (!is_last_step || is_first_step))
		{
			continue;
		}
		execute_one_resize_action(
			tmp_win, action[i], new_g, &old_g2, client_g, &b);
	}

	return;
}

static void get_resize_action_order(
	resize_action_type *action, rectangle *delta_g,
	resize_mode_type mode_x, resize_mode_type mode_y,
	Bool is_hidden_at_start, Bool is_hidden_at_end, Bool has_focus)
{
	int n = 0;
	Bool is_resizing;

	if (is_hidden_at_start && mode_x != RESIZE_MODE_RESIZE)
	{
		if ((delta_g->width != 0 && delta_g->height != 0) ||
		    !is_hidden_at_end)
		{
			/* The parent window becomes visible during the
			 * animation.  Make sure it is raised. */
			action[n++] = RA_FIRST_SETUP_HIDDEN;
			action[n++] = RA_FIRST_RAISE_PARENT;
		}
	}

	is_resizing = (mode_x == RESIZE_MODE_RESIZE) ? True : False;
	if (delta_g->width >= 0 && delta_g->height >= 0)
	{
		action[n++] = RA_X_RESIZE_TITLE;
		action[n++] = RA_Y_RESIZE_TITLE;
		action[n++] = RA_RESIZE_DECORW;
		if (is_resizing == True)
		{
			action[n++] = RA_RESIZE_CLIENT;
		}
		action[n++] = RA_RESIZE_PARENT;
		action[n++] = RA_MOVERESIZE_FRAME;
	}
	else if (delta_g->width >= 0 && delta_g->height < 0)
	{
		action[n++] = RA_X_RESIZE_TITLE;
		action[n++] = RA_X_RESIZE_DECORW;
		if (is_resizing == True)
		{
			action[n++] = RA_X_RESIZE_CLIENT;
		}
		action[n++] = RA_X_RESIZE_PARENT;
		action[n++] = RA_MOVERESIZE_FRAME;
		action[n++] = RA_Y_RESIZE_PARENT;
		if (is_resizing == True)
		{
			action[n++] = RA_Y_RESIZE_CLIENT;
		}
		action[n++] = RA_Y_RESIZE_DECORW;
		action[n++] = RA_Y_RESIZE_TITLE;
	}
	else if (delta_g->width < 0 && delta_g->height >= 0)
	{
		action[n++] = RA_Y_RESIZE_TITLE;
		action[n++] = RA_Y_RESIZE_DECORW;
		if (is_resizing == True)
		{
			action[n++] = RA_Y_RESIZE_CLIENT;
		}
		action[n++] = RA_Y_RESIZE_PARENT;
		action[n++] = RA_MOVERESIZE_FRAME;
		action[n++] = RA_X_RESIZE_PARENT;
		if (is_resizing == True)
		{
			action[n++] = RA_X_RESIZE_CLIENT;
		}
		action[n++] = RA_X_RESIZE_DECORW;
		action[n++] = RA_X_RESIZE_TITLE;
	}
	else if (delta_g->width < 0 && delta_g->height < 0)
	{
		action[n++] = RA_MOVERESIZE_FRAME;
		action[n++] = RA_RESIZE_PARENT;
		if (is_resizing == True)
		{
			action[n++] = RA_RESIZE_CLIENT;
		}
		action[n++] = RA_RESIZE_DECORW;
		action[n++] = RA_Y_RESIZE_TITLE;
		action[n++] = RA_X_RESIZE_TITLE;
	}
	if (is_hidden_at_end && mode_x != RESIZE_MODE_RESIZE)
	{
		action[n++] = RA_LAST_LOWER_PARENT;
	}
	if (has_focus && is_hidden_at_end)
	{
		/* domivogt (28-Dec-1999): For some reason the XMoveResize() on
		 * the frame window removes the input focus from the client
		 * window.  I have no idea why, but if we explicitly restore
		 * the focus here everything works fine. */
		action[n++] = RA_LAST_SETUP_HIDDEN;
		action[n++] = RA_LAST_RESTORE_FOCUS;
	}
	action[n++] = RA_DONE;

	return;
}

static void frame_setup_border(
	FvwmWindow *tmp_win, int w, int h)
{
	int add;
	int i;
	int is_shaded = IS_SHADED(tmp_win);
	XWindowChanges xwc;
	unsigned long xwcm;
	int xwidth;
	int ywidth;

	if (!HAS_BORDER(tmp_win))
	{
		return;
	}
	tmp_win->visual_corner_width = tmp_win->corner_width;
	if (w < 2*tmp_win->corner_width)
	{
		tmp_win->visual_corner_width = w / 3;
	}
	if (h < 2*tmp_win->corner_width && !is_shaded)
	{
		tmp_win->visual_corner_width = h / 3;
	}
	xwidth = w - 2 * tmp_win->visual_corner_width;
	ywidth = h - 2 * tmp_win->visual_corner_width;
	xwcm = CWWidth | CWHeight | CWX | CWY;
	if (xwidth<2)
	{
		xwidth = 2;
	}
	if (ywidth<2)
	{
		ywidth = 2;
	}

	if (is_shaded)
	{
		add = tmp_win->visual_corner_width / 3;
	}
	else
	{
		add = 0;
	}
	for (i = 0; i < 4; i++)
	{
		if (i == 0)
		{
			xwc.x = tmp_win->visual_corner_width;
			xwc.y = 0;
			xwc.height = tmp_win->boundary_width;
			xwc.width = xwidth;
		}
		else if (i == 1)
		{
			xwc.x = w - tmp_win->boundary_width;
			xwc.y = tmp_win->visual_corner_width;
			if (is_shaded)
			{
				xwc.y /= 3;
				xwc.height = add + 2;
			}
			else
			{
				xwc.height = ywidth;
			}
			xwc.width = tmp_win->boundary_width;

		}
		else if (i == 2)
		{
			xwc.x = tmp_win->visual_corner_width;
			xwc.y = h - tmp_win->boundary_width;
			xwc.height = tmp_win->boundary_width;
			xwc.width = xwidth;
		}
		else
		{
			xwc.x = 0;
			xwc.y = tmp_win->visual_corner_width;
			if (is_shaded)
			{
				xwc.y /= 3;
				xwc.height = add + 2;
			}
			else
			{
				xwc.height = ywidth;
			}
			xwc.width = tmp_win->boundary_width;
		}
		XConfigureWindow(
			dpy, tmp_win->sides[i], xwcm, &xwc);
	}

	xwcm = CWX|CWY|CWWidth|CWHeight;
	xwc.width = tmp_win->visual_corner_width;
	xwc.height = tmp_win->visual_corner_width;
	for (i = 0; i < 4; i++)
	{
		if (i & 0x1)
		{
			xwc.x = w -
				tmp_win->visual_corner_width;
		}
		else
		{
			xwc.x = 0;
		}

		if (i & 0x2)
		{
			xwc.y = h + add -
				tmp_win->visual_corner_width;
		}
		else
		{
			xwc.y = -add;
		}

		XConfigureWindow(
			dpy, tmp_win->corners[i], xwcm, &xwc);
	}

	return;
}

static void frame_setup_title_bar(
	FvwmWindow *tmp_win, int w, int h)
{
	XWindowChanges xwc;
	unsigned long xwcm;
	int i;
	int buttons = 0;
	int bsize;
	int wsize;
	int rest = 0;
	int bw;
	int title_off;
	int *px;
	int *py;
	int *pwidth;
	int *pheight;
	int space;
	rectangle tmp_g;
	size_borders b;

	if (!HAS_TITLE(tmp_win))
	{
		return;
	}

	get_window_borders(tmp_win, &b);
	if (HAS_VERTICAL_TITLE(tmp_win))
	{
		space = h;
		px = &xwc.y;
		py = &xwc.x;
		pwidth = &xwc.height;
		pheight = &xwc.width;
		bsize = b.total_size.height;
		wsize = tmp_win->frame_g.height - bsize;
	}
	else
	{
		space = w;
		px = &xwc.x;
		py = &xwc.y;
		pwidth = &xwc.width;
		pheight = &xwc.height;
		bsize = b.total_size.width;
		wsize = tmp_win->frame_g.width - bsize;
	}
	tmp_win->title_length = space -
		(tmp_win->nr_left_buttons + tmp_win->nr_right_buttons) *
		tmp_win->title_thickness - bsize;
	if (tmp_win->title_length < 1)
	{
		tmp_win->title_length = 1;
	}
	tmp_g.width = w;
	tmp_g.height = h;
	get_title_geometry(tmp_win, &tmp_g);
	xwcm = CWX | CWY | CWHeight | CWWidth;
	xwc.x = tmp_g.x;
	xwc.y = tmp_g.y;
	xwc.width = tmp_win->title_thickness;
	xwc.height = tmp_win->title_thickness;
	for (i = 0; i < NUMBER_OF_BUTTONS; i++)
	{
		if (tmp_win->button_w[i])
			buttons++;
	}
	if (wsize < buttons * *pwidth)
	{
		*pwidth = wsize / buttons;
		if (*pwidth < 1)
			*pwidth = 1;
		if (*pwidth > tmp_win->title_thickness)
			*pwidth = tmp_win->title_thickness;
		rest = wsize - buttons * *pwidth;
		if (rest > 0)
			(*pwidth)++;
	}
	/* left */
	title_off = *px;
	for (i = 0; i < NUMBER_OF_BUTTONS; i += 2)
	{
		if (tmp_win->button_w[i] != None)
		{
			if (*px + *pwidth < space - tmp_win->boundary_width)
			{
				XConfigureWindow(
					dpy, tmp_win->button_w[i], xwcm, &xwc);
				*px += *pwidth;
				title_off += *pwidth;
			}
			else
			{
				*px = -tmp_win->title_thickness;
				XConfigureWindow(
					dpy, tmp_win->button_w[i], xwcm, &xwc);
			}
			rest--;
			if (rest == 0)
			{
				(*pwidth)--;
			}
		}
	}
	bw = *pwidth;

	/* title */
	if (tmp_win->title_length <= 0 || wsize < 0)
		title_off = -10;
	*px = title_off;
	*pwidth = tmp_win->title_length;
	XConfigureWindow(dpy, tmp_win->title_w, xwcm, &xwc);

	/* right */
	*pwidth = bw;
	*px = space - tmp_win->boundary_width - *pwidth;
	for (i = 1 ; i < NUMBER_OF_BUTTONS; i += 2)
	{
		if (tmp_win->button_w[i] != None)
		{
			if (*px > tmp_win->boundary_width)
			{
				XConfigureWindow(
					dpy, tmp_win->button_w[i], xwcm, &xwc);
				*px -= *pwidth;
			}
			else
			{
				*px = -tmp_win->title_thickness;
				XConfigureWindow(
					dpy, tmp_win->button_w[i], xwcm, &xwc);
			}
			rest--;
			if (rest == 0)
			{
				(*pwidth)--;
				(*px)++;
			}
		}
	}

	return;
}

static void frame_setup_frame(
	FvwmWindow *tmp_win, rectangle *new_g)
{
	size_borders b;

	get_window_borders(tmp_win, &b);
	tmp_win->attr.width = new_g->width - b.total_size.width;
	tmp_win->attr.height = new_g->height - b.total_size.height;
	frame_resize(
		tmp_win, &tmp_win->frame_g, new_g, RESIZE_MODE_RESIZE,
		RESIZE_MODE_RESIZE, 0);
	tmp_win->frame_g = *new_g;

	return;
}

void frame_setup_window_internal(
	FvwmWindow *tmp_win, int x, int y, int w, int h,
	Bool do_send_configure_notify, Bool do_force)
{
	Bool is_resized = False;
	Bool is_moved = False;
	rectangle new_g;

	/* sanity checks */
	if (w < 1)
	{
		w = 1;
	}
	if (h < 1)
	{
		h = 1;
	}
	/* set some flags */
	if (do_force ||
	    w != tmp_win->frame_g.width || h != tmp_win->frame_g.height)
	{
		is_resized = True;
	}
	if (do_force || x != tmp_win->frame_g.x || y != tmp_win->frame_g.y)
	{
		is_moved = True;
	}
	/* setup the window */
	new_g.x = x;
	new_g.y = y;
	new_g.width = w;
	new_g.height = h;
	frame_setup_frame(tmp_win, &new_g);
	if (is_resized)
	{
		frame_setup_title_bar(tmp_win, w, h);
		frame_setup_border(tmp_win, w, h);
		if (FShapesSupported && tmp_win->wShaped)
		{
			frame_setup_shape(tmp_win, w, h);
		}
	}
	/* inform the application of the change
	 *
	 * According to the July 27, 1988 ICCCM draft, we should send a
	 * synthetic ConfigureNotify event to the client if the window was
	 * moved but not resized. */
	if (is_moved && !is_resized)
	{
		do_send_configure_notify = True;
	}
	/* must not send events to shaded windows because this might cause them
	 * to look at their current geometry */
	if (do_send_configure_notify && !IS_SHADED(tmp_win))
	{
		SendConfigureNotify(tmp_win, x, y, w, h, 0, True);
	}
	/* get things updated */
	XSync(dpy, 0);
	/* inform the modules of the change */
	BroadcastConfig(M_CONFIGURE_WINDOW,tmp_win);

#if 0
	fprintf(stderr, "sf: window '%s'\n", tmp_win->name);
	fprintf(stderr, "    frame:  %5d %5d, %4d x %4d\n", tmp_win->frame_g.x,
		tmp_win->frame_g.y, tmp_win->frame_g.width,
		tmp_win->frame_g.height);
	fprintf(stderr, "    normal: %5d %5d, %4d x %4d\n",
		tmp_win->normal_g.x, tmp_win->normal_g.y,
		tmp_win->normal_g.width, tmp_win->normal_g.height);
	if (IS_MAXIMIZED(tmp_win))
	{
		fprintf(stderr, "    max:    %5d %5d, %4d x %4d, %5d %5d\n",
			tmp_win->max_g.x, tmp_win->max_g.y,
			tmp_win->max_g.width, tmp_win->max_g.height,
			tmp_win->max_offset.x, tmp_win->max_offset.y);
	}
#endif

	return;
}

/* ---------------------------- interface functions ------------------------- */

void frame_resize(
	FvwmWindow *tmp_win, rectangle *start_g, rectangle *end_g,
	resize_mode_type mode_x, resize_mode_type mode_y, int anim_steps)
{
	rectangle client_g;
	int nstep;
#if 0
	int step = 1;
	int move_parent_too = False;
	rectangle frame_g;
	rectangle parent_g;
	rectangle shape_g;
#endif
	rectangle delta_g;
#if 0
	rectangle diff;
	rectangle pdiff;
	rectangle sdiff;
	size_borders b;
#endif
	FvwmWindow *sf;
	static Window shape_w = None;
	decor_gravities_type grav;
	resize_action_type action[32];
	rectangle current_g;
	rectangle next_g;

print_g("start", start_g);
print_g("end  ", end_g);
	/* prepare a shape window if necessary */
	if (FShapesSupported && shape_w == None && tmp_win->wShaped)
	{
		XSetWindowAttributes attributes;
		unsigned long valuemask;

		valuemask = CWOverrideRedirect;
		attributes.override_redirect = True;
		shape_w = XCreateWindow(
			dpy, Scr.Root, -32768, -32768, 1, 1, 0, CopyFromParent,
			(unsigned int)CopyFromParent, CopyFromParent,
			valuemask, &attributes);
	}
	/* sanity checks */
	if (end_g->width < 1)
	{
		end_g->width = 1;
	}
	if (end_g->height < 1)
	{
		end_g->height = 1;
	}
	if (mode_x == RESIZE_MODE_RESIZE || mode_y == RESIZE_MODE_RESIZE)
	{
		/* no animated resizing */
		anim_steps = 0;
		mode_x = RESIZE_MODE_RESIZE;
		mode_y = RESIZE_MODE_RESIZE;
	}
	/* calculate diff of size and position */
	delta_g.x = end_g->x - start_g->x;
	delta_g.y = end_g->y - start_g->y;
	delta_g.width = end_g->width - start_g->width;
	delta_g.height = end_g->height - start_g->height;
	/* calcuate the number of steps */
	{
		int whdiff;

		whdiff = max(abs(delta_g.width), abs(delta_g.height));
		if (whdiff == 0 && mode_x != RESIZE_MODE_RESIZE)
		{
			return;
		}
		if (anim_steps < 0)
		{
			anim_steps = (whdiff - 1) / -anim_steps;
		}
		else if (anim_steps > 0 && anim_steps >= whdiff)
		{
			anim_steps = whdiff - 1;
		}
		anim_steps++;
	}
	/* set gravities for the window parts */
	frame_get_resize_decor_gravities(
		&grav, GET_TITLE_DIR(tmp_win), mode_x, mode_y);
	frame_set_decor_gravities(tmp_win, &grav);
#if 0
	if (anim_steps > 1)
	{
		move_parent_too = HAS_TITLE_DIR(tmp_win, DIR_S);
	}
#endif
	/* calculate order in which to do things */
	sf = get_focus_window();
	get_resize_action_order(
		&(action[0]), &delta_g, mode_x, mode_y,
		is_hidden(tmp_win, start_g), is_hidden(tmp_win, end_g),
		(sf == tmp_win) ? True : False);
	/* prepare some convenience variables */
#if 0
	frame_g = tmp_win->frame_g;
#endif
	get_client_geometry(tmp_win, &client_g);

	/* animation */
	for (nstep = 1, current_g = tmp_win->frame_g; nstep <= anim_steps;
	     nstep++, current_g = next_g)
	{
		next_g = *start_g;
		next_g.x += (delta_g.x * nstep) / anim_steps;
		next_g.y += (delta_g.y * nstep) / anim_steps;
		next_g.width += (delta_g.width * nstep) / anim_steps;
		next_g.height += (delta_g.height * nstep) / anim_steps;
		execute_resize_actions(
			tmp_win, &(action[0]), &next_g, &current_g, &client_g,
			(nstep == 1) ? True : False,
			(nstep == anim_steps) ? True : False);
	}








#if 0
	if (anim_steps == 1)
	{
		/*!!!*/
		/*!!! unshade*/
		{
			XMoveResizeWindow(
				dpy, tmp_win->w, 0, 0, client_g.width,
				client_g.height);
			if (delta_g.x > 0)
			{
				XMoveResizeWindow(
					dpy, tmp_win->Parent, b.top_left.width,
					b.top_left.height - client_g.height + 1,
					client_g.width, client_g.height);
#if 0
				frame_set_decor_gravities(
					tmp_win, SouthEastGravity,
					SouthEastGravity, NorthWestGravity);
#endif
			}
			else
			{
				XMoveResizeWindow(
					dpy, tmp_win->Parent, b.top_left.width,
					b.top_left.height, client_g.width,
					client_g.height);
#if 0
				set_decor_gravity(
					tmp_win, NorthWestGravity,
					NorthWestGravity, NorthWestGravity);
#endif
			}
			XLowerWindow(dpy, tmp_win->decor_w);
			XMoveResizeWindow(
				dpy, tmp_win->frame, end_g->x, end_g->y,
				end_g->width, end_g->height);
			tmp_win->frame_g.height = end_g->height + 1;
		}
		/*!!! shade*/
		{
			/* All this stuff is necessary to prevent flickering */
#if 0
			set_decor_gravity(
				tmp_win, title_grav, UnmapGravity, client_grav);
#endif
			XMoveResizeWindow(
				dpy, tmp_win->frame, end_g->x, end_g->y,
				end_g->width, end_g->height);
			XResizeWindow(dpy, tmp_win->Parent, client_g.width, 1);
			XMoveWindow(dpy, tmp_win->decor_w, 0, 0);
			XRaiseWindow(dpy, tmp_win->decor_w);
			XMapWindow(dpy, tmp_win->Parent);

			if (delta_g.x > 0)
			{
				XMoveWindow(dpy, tmp_win->w, 0,
					    1 - client_g.height);
			}
			else
			{
				XMoveWindow(dpy, tmp_win->w, 0, 0);
			}

			/* domivogt (28-Dec-1999): For some reason the
			 * XMoveResize() on the frame window removes the input
			 * focus from the client window.  I have no idea why,
			 * but if we explicitly restore the focus here
			 * everything works fine.
			 */
			if (sf == tmp_win)
			{
				FOCUS_SET(tmp_win->w);
			}
		}
	}
#endif




	/*  post animation actions */
	if (IS_SHADED(tmp_win) && mode_x != RESIZE_MODE_RESIZE)
	{
		/* hide the parent window below the decor window */
		XLowerWindow(dpy, tmp_win->Parent);
	}
#if 0
	set_decor_gravity(
		tmp_win, NorthWestGravity, NorthWestGravity, NorthWestGravity);
	/* Finally let frame_setup_window take care of the window */
	frame_setup_window(
		tmp_win, end_g->x, end_g->y, end_g->width, end_g->height, True);
#endif
	tmp_win->frame_g = *end_g;
	/* clean up */
	update_absolute_geometry(tmp_win);
	if (delta_g.width != 0 || delta_g.height != 0 ||
	    mode_x != RESIZE_MODE_RESIZE)
	{
		DrawDecorations(
			tmp_win, DRAW_FRAME | DRAW_BUTTONS,
			(Scr.Hilite == tmp_win), True, None, CLEAR_NONE);
	}

	return;
}

/* Calculates the gravities for the various parts of the decor through ret_grav.
 * This can be passed to frame_set_decor_gravities.
 *
 * title_dir
 *   The direction of the title in the frame.
 * mode_x
 *   The mode for the resize operation in x direction
 * mode_y
 *   The mode for the resize operation in y direction
 */
void frame_get_resize_decor_gravities(
	decor_gravities_type *ret_grav, direction_type title_dir,
	resize_mode_type mode_x, resize_mode_type mode_y)
{
	decor_gravities_type grav_x;
	decor_gravities_type grav_y;
	direction_type title_dir_x;
	direction_type title_dir_y;

	gravity_split_xy_dir(&title_dir_x, &title_dir_y, title_dir);
	get_resize_decor_gravities_one_axis(
		&grav_x, title_dir_x, mode_x, DIR_W, DIR_E);
	get_resize_decor_gravities_one_axis(
		&grav_y, title_dir_y, mode_y, DIR_N, DIR_S);
	combine_decor_gravities(ret_grav, &grav_x, &grav_y);

	return;
}

/* sets the gravity for the various parts of the window */
void frame_set_decor_gravities(
	FvwmWindow *tmp_win, decor_gravities_type *grav)
{
	int valuemask = CWWinGravity;
	XSetWindowAttributes xcwa;
	int i;

	xcwa.win_gravity = grav->client_grav;
	XChangeWindowAttributes(dpy, tmp_win->w, valuemask, &xcwa);
	xcwa.win_gravity = grav->parent_grav;
	XChangeWindowAttributes(dpy, tmp_win->Parent, valuemask, &xcwa);
	xcwa.win_gravity = grav->decor_grav;
	XChangeWindowAttributes(dpy, tmp_win->decor_w, valuemask, &xcwa);
	if (!HAS_TITLE(tmp_win))
	{
		return;
	}
	xcwa.win_gravity = grav->title_grav;
	XChangeWindowAttributes(dpy, tmp_win->title_w, valuemask, &xcwa);
	xcwa.win_gravity = grav->lbutton_grav;
	for (i = 0; i < NUMBER_OF_BUTTONS; i += 2)
	{
		if (tmp_win->button_w[i])
		{
			XChangeWindowAttributes(
				dpy, tmp_win->button_w[i], valuemask, &xcwa);
		}
	}
	xcwa.win_gravity = grav->rbutton_grav;
	for (i = 1; i < NUMBER_OF_BUTTONS; i += 2)
	{
		if (tmp_win->button_w[i])
		{
			XChangeWindowAttributes(
				dpy, tmp_win->button_w[i], valuemask, &xcwa);
		}
	}

	return;
}

/***********************************************************************
 *
 *  Procedure:
 *      frame_setup_window - set window sizes
 *
 *  Inputs:
 *      tmp_win - the FvwmWindow pointer
 *      x       - the x coordinate of the upper-left outer corner of the frame
 *      y       - the y coordinate of the upper-left outer corner of the frame
 *      w       - the width of the frame window w/o border
 *      h       - the height of the frame window w/o border
 *
 *  Special Considerations:
 *      This routine will check to make sure the window is not completely
 *      off the display, if it is, it'll bring some of it back on.
 *
 *      The tmp_win->frame_XXX variables should NOT be updated with the
 *      values of x,y,w,h prior to calling this routine, since the new
 *      values are compared against the old to see whether a synthetic
 *      ConfigureNotify event should be sent.  (It should be sent if the
 *      window was moved but not resized.)
 *
 ************************************************************************/
void frame_setup_window(
	FvwmWindow *tmp_win, int x, int y, int w, int h,
	Bool do_send_configure_notify)
{
	frame_setup_window_internal(
		tmp_win, x, y, w, h, do_send_configure_notify, False);

	return;
}

void frame_force_setup_window(
	FvwmWindow *tmp_win, int x, int y, int w, int h,
	Bool do_send_configure_notify)
{
	frame_setup_window_internal(
		tmp_win, x, y, w, h, do_send_configure_notify, True);

	return;
}

/****************************************************************************
 *
 * Sets up the shaped window borders
 *
 ****************************************************************************/
void frame_setup_shape(FvwmWindow *tmp_win, int w, int h)
{
	XRectangle rect;
	rectangle r;
	size_borders b;

	if (!FShapesSupported)
	{
		return;
	}
	if (!tmp_win->wShaped)
	{
		/* unset shape */
		FShapeCombineMask(
			dpy, tmp_win->frame, FShapeBounding, 0, 0, None,
			FShapeSet);
	}
	/* shape the window */
	get_window_borders(tmp_win, &b);
	FShapeCombineShape(
		dpy, tmp_win->frame, FShapeBounding, b.top_left.width,
		b.top_left.height, tmp_win->w, FShapeBounding, FShapeSet);
	if (tmp_win->title_w)
	{
		/* windows w/ titles */
		r.width = w;
		r.height = h;
		get_title_geometry(tmp_win, &r);
		rect.x = r.x;
		rect.y = r.y;
		rect.width = r.width;
		rect.height = r.height;
		FShapeCombineRectangles(
			dpy, tmp_win->frame, FShapeBounding, 0, 0, &rect, 1,
			FShapeUnion, Unsorted);
	}

	return;
}

/* ---------------------------- builtin commands ---------------------------- */























#if 0
	else
	{
		/*!!!*/
		/*!!! unshade*/
		{
			XMoveResizeWindow(
				dpy, tmp_win->Parent, b.top_left.width, b.top_left.height, client_g.width, 1);
			XMoveWindow(
				dpy, tmp_win->w, 0,
				(client_grav == SouthEastGravity) ? -client_g.height + 1 : 0);
			XLowerWindow(dpy, tmp_win->decor_w);
			parent_g.x = b.top_left.width;
			parent_g.y = b.top_left.height +
				((delta_g.x > 0) ? -step : 0);
			parent_g.width = client_g.width;
			parent_g.height = 0;
			pdiff.x = 0;
			pdiff.y = 0;
			pdiff.width = 0;
			pdiff.height = step;
			if (client_grav == SouthEastGravity)
			{
				shape_g.x = b.top_left.width;
				shape_g.y = b.top_left.height - client_g.height;
				sdiff.x = 0;
				sdiff.y = step;
			}
			else
			{
				shape_g.x = b.top_left.width;
				shape_g.y = b.top_left.height;
				sdiff.x = 0;
				sdiff.y = 0;
			}
			diff.x = 0;
			diff.y = (delta_g.x > 0) ? -step : 0;
			diff.width = 0;
			diff.height = step;

			/* This causes a ConfigureNotify if the client size has changed while
			 * the window was shaded. */
			XResizeWindow(dpy, tmp_win->w, client_g.width, client_g.height);
			/* make the decor window the full size before the animation unveils it */
			XMoveResizeWindow(
				dpy, tmp_win->decor_w, 0, (delta_g.x > 0) ?
				frame_g.height - end_g->height : 0, end_g->width, end_g->height);
			tmp_win->frame_g.width = end_g->width;
			tmp_win->frame_g.height = end_g->height;
			/* draw the border decoration iff backing store is on */
			if (Scr.use_backing_store != NotUseful)
			{ /* eek! fvwm doesn't know the backing_store setting for windows */
				XWindowAttributes xwa;

				if (XGetWindowAttributes(dpy, tmp_win->decor_w, &xwa)
				    && xwa.backing_store != NotUseful)
				{
					DrawDecorations(
						tmp_win, DRAW_FRAME, sf == tmp_win, True, None, CLEAR_ALL);
				}
			}
			while (frame_g.height + diff.height < end_g->height)
			{
				parent_g.x += pdiff.x;
				parent_g.y += pdiff.y;
				parent_g.width += pdiff.width;
				parent_g.height += pdiff.height;
				shape_g.x += sdiff.x;
				shape_g.y += sdiff.y;
				frame_g.x += diff.x;
				frame_g.y += diff.y;
				frame_g.width += diff.width;
				frame_g.height += diff.height;
				if (FShapesSupported && tmp_win->wShaped && shape_w)
				{
					FShapeCombineShape(
						dpy, shape_w, FShapeBounding, shape_g.x, shape_g.y, tmp_win->w,
						FShapeBounding, FShapeSet);
					if (tmp_win->title_w)
					{
						XRectangle rect;

						/* windows w/ titles */
						rect.x = b.top_left.width;
						rect.y = (delta_g.x > 0) ?
							frame_g.height - b.bottom_right.height : 0;
						rect.width = frame_g.width - b.total_size.width;
						rect.height = tmp_win->title_thickness;
						FShapeCombineRectangles(
							dpy, shape_w, FShapeBounding, 0, 0, &rect, 1, FShapeUnion,
							Unsorted);
					}
				}
				if (move_parent_too)
				{
					if (FShapesSupported && tmp_win->wShaped && shape_w)
					{
						FShapeCombineShape(
							dpy, tmp_win->frame, FShapeBounding, 0, 0, shape_w,
							FShapeBounding, FShapeUnion);
					}
					XMoveResizeWindow(
						dpy, tmp_win->Parent, parent_g.x, parent_g.y, parent_g.width,
						parent_g.height);
				}
				else
				{
					XResizeWindow(dpy, tmp_win->Parent, parent_g.width, parent_g.height);
				}
				XMoveResizeWindow(
					dpy, tmp_win->frame, frame_g.x, frame_g.y, frame_g.width,
					frame_g.height);
				if (FShapesSupported && tmp_win->wShaped && shape_w)
				{
					FShapeCombineShape(
						dpy, tmp_win->frame, FShapeBounding, 0, 0, shape_w, FShapeBounding,
						FShapeSet);
				}
				DrawDecorations(
					tmp_win, DRAW_FRAME, sf == tmp_win, True, None, CLEAR_NONE);
				FlushAllMessageQueues();
				XSync(dpy, 0);
			}
			if (client_grav == SouthEastGravity && move_parent_too)
			{
				/* We must finish above loop to the very end for this special case.
				 * Otherwise there is a visible jump of the client window. */
				XMoveResizeWindow(
					dpy, tmp_win->Parent, parent_g.x,
					b.top_left.height - (end_g->height - frame_g.height), parent_g.width,
					client_g.height);
				XMoveResizeWindow(
					dpy, tmp_win->frame, end_g->x, end_g->y, end_g->width, end_g->height);
			}
			else
			{
				XMoveWindow(dpy, tmp_win->w, 0, 0);
			}
			if (sf)
			{
				focus_grab_buttons(sf, True);
			}
			tmp_win->frame_g.height = end_g->height + 1;
		}
		/*!!! shade*/
		{
			parent_g.x = b.top_left.width;
			parent_g.y = b.top_left.height;
			parent_g.width = client_g.width;
			parent_g.height = frame_g.height - b.total_size.height;
			pdiff.x = 0;
			pdiff.y = 0;
			pdiff.width = 0;
			pdiff.height = -step;
			shape_g = parent_g;
			sdiff.x = 0;
			sdiff.y = (client_grav == SouthEastGravity) ? -step : 0;
			diff.x = 0;
			diff.y = (delta_g.x > 0) ? step : 0;
			diff.width = 0;
			diff.height = -step;

			while (frame_g.height + diff.height > end_g->height)
			{
				parent_g.x += pdiff.x;
				parent_g.y += pdiff.y;
				parent_g.width += pdiff.width;
				parent_g.height += pdiff.height;
				shape_g.x += sdiff.x;
				shape_g.y += sdiff.y;
				frame_g.x += diff.x;
				frame_g.y += diff.y;
				frame_g.width += diff.width;
				frame_g.height += diff.height;
				if (FShapesSupported && tmp_win->wShaped &&
				    shape_w)
				{
					FShapeCombineShape(
						dpy, shape_w, FShapeBounding,
						shape_g.x, shape_g.y,
						tmp_win->w, FShapeBounding,
						FShapeSet);
					if (tmp_win->title_w)
					{
						XRectangle rect;

						/* windows w/ titles */
						rect.x = b.top_left.width;
						rect.y = (delta_g.x > 0) ?
							frame_g.height -
							b.bottom_right.height :
							0;
						rect.width =
							start_g->width -
							b.total_size.width;
						rect.height =
							tmp_win->title_thickness;
						FShapeCombineRectangles(
							dpy, shape_w,
							FShapeBounding, 0, 0,
							&rect, 1, FShapeUnion,
							Unsorted);
					}
				}
				if (move_parent_too)
				{
					if (FShapesSupported &&
					    tmp_win->wShaped && shape_w)
					{
						FShapeCombineShape(
							dpy, tmp_win->frame,
							FShapeBounding, 0, 0,
							shape_w, FShapeBounding,
							FShapeUnion);
					}
					XMoveResizeWindow(
						dpy, tmp_win->frame,
						frame_g.x, frame_g.y,
						frame_g.width, frame_g.height);
					XMoveResizeWindow(
						dpy, tmp_win->Parent,
						parent_g.x, parent_g.y,
						parent_g.width,
						parent_g.height);
				}
				else
				{
					XResizeWindow(
						dpy, tmp_win->Parent,
						parent_g.width,
						parent_g.height);
					XMoveResizeWindow(
						dpy, tmp_win->frame, frame_g.x,
						frame_g.y, frame_g.width,
						frame_g.height);
				}
				if (FShapesSupported && tmp_win->wShaped &&
				    shape_w)
				{
					FShapeCombineShape(
						dpy, tmp_win->frame,
						FShapeBounding, 0, 0, shape_w,
						FShapeBounding, FShapeSet);
				}
				FlushAllMessageQueues();
				XSync(dpy, 0);
			}
			/* All this stuff is necessary to prevent flickering */
			set_decor_gravity(
				tmp_win, title_grav, UnmapGravity, client_grav);
			XMoveResizeWindow(
				dpy, tmp_win->frame, end_g->x, end_g->y,
				end_g->width, end_g->height);
			XResizeWindow(dpy, tmp_win->Parent, client_g.width, 1);
			XMoveWindow(dpy, tmp_win->decor_w, 0, 0);
			XRaiseWindow(dpy, tmp_win->decor_w);
			XMapWindow(dpy, tmp_win->Parent);

			if (delta_g.x > 0)
			{
				XMoveWindow(
					dpy, tmp_win->w, 0,
					1 - client_g.height);
			}
			else
			{
				XMoveWindow(dpy, tmp_win->w, 0, 0);
			}

			/* domivogt (28-Dec-1999): For some reason the
			 * XMoveResize() on the frame window removes the input
			 * focus from the client window.  I have no idea why,
			 * but if we explicitly restore the focus here
			 * everything works fine. */
			if (sf == tmp_win)
			{
				FOCUS_SET(tmp_win->w);
			}
		} /* shade */
	}
#endif
