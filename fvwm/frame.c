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
#include <signal.h>

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/Grab.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "events.h"
#include "misc.h"
#include "screen.h"
#include "geometry.h"
#include "module_interface.h"
#include "focus.h"
#include "borders.h"
#include "frame.h"
#include "ewmh.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

typedef struct
{
	int decor_grav;
	int title_grav;
	int lbutton_grav;
	int rbutton_grav;
	int parent_grav;
	int client_grav;
} frame_decor_gravities_type;

typedef struct
{
	/* filled when args are created */
	frame_move_resize_mode mode;
	frame_decor_gravities_type grav;
	size_borders b_g;
	size_borders b_no_title_g;
	rectangle curr_sidebar_g;
	rectangle start_g;
	rectangle end_g;
	rectangle delta_g;
	rectangle current_g;
	rectangle client_g;
	int anim_steps;
	Window w_with_focus;
	int current_step;
	int curr_titlebar_compression;
	direction_t shade_dir;
	window_parts trans_parts;
	struct
	{
		unsigned do_force : 1;
		unsigned do_not_configure_client : 1;
		unsigned do_not_draw : 1;
		unsigned do_restore_gravity : 1;
		unsigned do_set_bit_gravity : 1;
		unsigned do_update_shape : 1;
		unsigned had_handles : 1;
		unsigned is_lazy_shading : 1;
		unsigned is_setup : 1;
		unsigned is_shading : 1;
		unsigned was_moved : 1;
	} flags;
	/* used during the animation */
	int next_titlebar_compression;
	rectangle next_sidebar_g;
	rectangle next_g;
	rectangle dstep_g;
	size_rect parent_s;
	int minimal_w_offset;
	int minimal_h_offset;
	struct
	{
		/* cleared before each step */
		unsigned do_hide_parent : 1;
		unsigned do_unhide_parent : 1;
		unsigned is_hidden : 1;
		unsigned was_hidden : 1;
	} step_flags;
} mr_args_internal;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* windows used to hide animation steps */
static struct
{
	Window parent;
	Window w[4];
} hide_wins;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

#if 0
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
#endif

static void combine_gravities(
	frame_decor_gravities_type *ret_grav,
	frame_decor_gravities_type *grav_x,
	frame_decor_gravities_type *grav_y)
{
	ret_grav->decor_grav = gravity_combine_xy_grav(
		grav_x->decor_grav, grav_y->decor_grav);
	ret_grav->title_grav = gravity_combine_xy_grav(
		grav_x->title_grav, grav_y->title_grav);
	ret_grav->lbutton_grav = gravity_combine_xy_grav(
		grav_x->lbutton_grav, grav_y->lbutton_grav);
	ret_grav->rbutton_grav = gravity_combine_xy_grav(
		grav_x->rbutton_grav, grav_y->rbutton_grav);
	ret_grav->parent_grav = gravity_combine_xy_grav(
		grav_x->parent_grav, grav_y->parent_grav);
	ret_grav->client_grav = gravity_combine_xy_grav(
		grav_x->client_grav, grav_y->client_grav);

	return;
}

static void get_resize_decor_gravities_one_axis(
	frame_decor_gravities_type *ret_grav, direction_t title_dir,
	frame_move_resize_mode axis_mode, direction_t neg_dir,
	direction_t pos_dir, int is_moving)
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
	switch (axis_mode)
	{
	case FRAME_MR_SCROLL:
		ret_grav->client_grav = (is_moving) ? neg_grav : pos_grav;
		break;
	case FRAME_MR_SHRINK:
		ret_grav->client_grav = (is_moving) ? pos_grav : neg_grav;
		break;
	case FRAME_MR_OPAQUE:
	case FRAME_MR_FORCE_SETUP:
	case FRAME_MR_FORCE_SETUP_NO_W:
	case FRAME_MR_SETUP:
	case FRAME_MR_SETUP_BY_APP:
		ret_grav->client_grav = neg_grav;
		break;
	case FRAME_MR_DONT_DRAW:
	default:
		/* can not happen, just a dummy to keep -Wall happy */
		ret_grav->client_grav = pos_grav;
		break;
	}
	ret_grav->parent_grav = ret_grav->client_grav;

	return;
}

static void frame_get_titlebar_dimensions_only(
	FvwmWindow *fw, rectangle *frame_g, size_borders *bs,
	rectangle *ret_titlebar_g)
{
	if (!HAS_TITLE(fw))
	{
		return;
	}
	switch (GET_TITLE_DIR(fw))
	{
	case DIR_W:
	case DIR_E:
		ret_titlebar_g->x = (GET_TITLE_DIR(fw) == DIR_W) ?
			bs->top_left.width :
			frame_g->width - bs->bottom_right.width -
			fw->title_thickness;
		ret_titlebar_g->y = bs->top_left.height;
		ret_titlebar_g->width = fw->title_thickness;
		ret_titlebar_g->height =
			frame_g->height - bs->total_size.height;
		break;
	case DIR_N:
	case DIR_S:
	default: /* default makes gcc4 happy */
		ret_titlebar_g->y = (GET_TITLE_DIR(fw) == DIR_N) ?
			bs->top_left.height :
			frame_g->height - bs->bottom_right.height -
			fw->title_thickness;
		ret_titlebar_g->x = bs->top_left.width;
		ret_titlebar_g->width = frame_g->width - bs->total_size.width;
		ret_titlebar_g->height = fw->title_thickness;
		break;
	}

	return;
}

static void frame_setup_border(
	FvwmWindow *fw, rectangle *frame_g, window_parts setup_parts,
	rectangle *diff_g)
{
	XWindowChanges xwc;
	Window w;
	window_parts part;
	rectangle sidebar_g;
	rectangle part_g;
	Bool dummy;

	if (HAS_NO_BORDER(fw))
	{
		return;
	}
	frame_get_sidebar_geometry(
		fw, NULL, frame_g, &sidebar_g, &dummy, &dummy);
	for (part = PART_BORDER_N; (part & PART_FRAME);
	     part <<= 1)
	{
		if ((part & PART_FRAME & setup_parts) == PART_NONE)
		{
			continue;
		}
		border_get_part_geometry(fw, part, &sidebar_g, &part_g, &w);
		if (part_g.width <= 0 || part_g.height <= 0)
		{
			xwc.x = -1;
			xwc.y = -1;
			xwc.width = 1;
			xwc.height = 1;
		}
		else
		{
			xwc.x = part_g.x;
			xwc.y = part_g.y;
			xwc.width = part_g.width;
			xwc.height = part_g.height;
		}
		if (diff_g != NULL)
		{
			if (part == PART_BORDER_NE || part == PART_BORDER_E ||
			    part == PART_BORDER_SE)
			{
				xwc.x -= diff_g->width;
			}
			if (part == PART_BORDER_SW || part == PART_BORDER_S ||
			    part == PART_BORDER_SE)
			{
				xwc.y -= diff_g->height;
			}
		}
		XConfigureWindow(dpy, w, CWWidth | CWHeight | CWX | CWY, &xwc);
	}

	return;
}

static void frame_setup_titlebar(
	FvwmWindow *fw, rectangle *frame_g, window_parts setup_parts,
	rectangle *diff_g)
{
	frame_title_layout_t title_layout;
	int i;

	if (!HAS_TITLE(fw))
	{
		return;
	}
	frame_get_titlebar_dimensions(fw, frame_g, diff_g, &title_layout);
	/* configure buttons */
	for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i++)
	{
		if (FW_W_BUTTON(fw, i) != None && (setup_parts & PART_BUTTONS))
		{
			XMoveResizeWindow(
				dpy, FW_W_BUTTON(fw, i),
				title_layout.button_g[i].x,
				title_layout.button_g[i].y,
				title_layout.button_g[i].width,
				title_layout.button_g[i].height);
		}
	}
	/* configure title */
	if (setup_parts & PART_TITLE)
	{
		XMoveResizeWindow(
			dpy, FW_W_TITLE(fw),
			title_layout.title_g.x, title_layout.title_g.y,
			title_layout.title_g.width,
			title_layout.title_g.height);
	}

	return;
}

static void __frame_setup_window(
	FvwmWindow *fw, rectangle *frame_g, Bool do_send_configure_notify,
	Bool do_force, Bool is_application_request)
{
	frame_move_resize_args mr_args;
	Bool is_resized = False;
	Bool is_moved = False;
	rectangle new_g;

	new_g = *frame_g;
	/* sanity checks */
	if (new_g.width < 1)
	{
		new_g.width = 1;
	}
	if (new_g.height < 1)
	{
		new_g.height = 1;
	}
	/* set some flags */
	if (new_g.width != fw->g.frame.width ||
	    new_g.height != fw->g.frame.height)
	{
		is_resized = True;
	}
	if (new_g.x != fw->g.frame.x || new_g.y != fw->g.frame.y)
	{
		is_moved = True;
	}
	/* setup the window */
	if (is_resized || do_force)
	{
		frame_move_resize_mode mode;

		if (is_application_request)
		{
			mode = FRAME_MR_SETUP_BY_APP;
		}
		else if (do_force)
		{
			mode = FRAME_MR_FORCE_SETUP;
		}
		else
		{
			mode = FRAME_MR_SETUP;
		}
		mr_args = frame_create_move_resize_args(
			fw, mode, NULL, &new_g, 0, DIR_NONE);
		frame_move_resize(fw, mr_args);
		((mr_args_internal *)mr_args)->flags.was_moved = 0;
		frame_free_move_resize_args(fw, mr_args);
		fw->g.frame = *frame_g;
	}
	else if (is_moved)
	{
		unsigned int draw_parts = PART_NONE;

		/* inform the application of the change
		 *
		 * According to the July 27, 1988 ICCCM draft, we should send a
		 * synthetic ConfigureNotify event to the client if the window
		 * was moved but not resized. */
		XMoveWindow(dpy, FW_W_FRAME(fw), frame_g->x, frame_g->y);
		fw->g.frame = *frame_g;
		if ((draw_parts = border_get_transparent_decorations_part(fw))
		    != PART_NONE)
		{
			border_draw_decorations(
				fw, draw_parts,
				((fw == get_focus_window())) ? True : False,
				True, CLEAR_ALL, NULL, NULL);
		}
		fw->g.frame = *frame_g;
		do_send_configure_notify = True;
	}
	/* must not send events to shaded windows because this might cause them
	 * to look at their current geometry */
	if (do_send_configure_notify && !IS_SHADED(fw))
	{
		SendConfigureNotify(
			fw, new_g.x, new_g.y, new_g.width, new_g.height, 0,
			True);
	}
	/* get things updated */
	XFlush(dpy);
	if (is_moved || is_resized)
	{
		/* inform the modules of the change */
		BroadcastConfig(M_CONFIGURE_WINDOW,fw);
	}

	return;
}

static void frame_reparent_hide_windows(
	Window w)
{
	int i;

	hide_wins.parent = w;
	for (i = 0; i < 4 ; i++)
	{
		if (w == Scr.Root)
		{
			XUnmapWindow(dpy, hide_wins.w[i]);
		}
		XReparentWindow(dpy, hide_wins.w[i], w, -1, -1);
	}
	if (w != Scr.Root)
	{
		XRaiseWindow(dpy, hide_wins.w[0]);
		XRestackWindows(dpy, hide_wins.w, 4);
	}

	return;
}

/* Returns True if the frame is so small that the parent window would have a
 * width or height smaller than one pixel. */
static Bool frame_is_parent_hidden(
	FvwmWindow *fw, rectangle *frame_g)
{
	size_borders b;

	get_window_borders(fw, &b);
	if (frame_g->width <= b.total_size.width ||
	    frame_g->height <= b.total_size.height)
	{
		return True;
	}

	return False;
}

/* Returns the number of pixels that the title bar is too short to accomodate
 * all the title buttons and a title window that has at least a length of one
 * pixel. */
static int frame_get_titlebar_compression(
	FvwmWindow *fw, rectangle *frame_g)
{
	size_borders b;
	int space;
	int need_space;

	if (!HAS_TITLE(fw))
	{
		return 0;
	}
	get_window_borders(fw, &b);
	if (HAS_VERTICAL_TITLE(fw))
	{
		space = frame_g->height - b.total_size.height;
	}
	else
	{
		space = frame_g->width - b.total_size.width;
	}
	need_space = (fw->nr_left_buttons + fw->nr_right_buttons) *
		fw->title_thickness + MIN_WINDOW_TITLE_LENGTH;
	if (space < need_space)
	{
		return need_space - space;
	}

	return 0;
}

/* Calculates the gravities for the various parts of the decor through ret_grav.
 * This can be passed to frame_set_decor_gravities.
 *
 * title_dir
 *   The direction of the title in the frame.
 * rmode
 *   The mode for the resize operation
 */
static void frame_get_resize_decor_gravities(
	frame_decor_gravities_type *ret_grav, direction_t title_dir,
	frame_move_resize_mode rmode, rectangle *delta_g)
{
	frame_decor_gravities_type grav_x;
	frame_decor_gravities_type grav_y;
	direction_t title_dir_x;
	direction_t title_dir_y;

	gravity_split_xy_dir(&title_dir_x, &title_dir_y, title_dir);
	get_resize_decor_gravities_one_axis(
		&grav_x, title_dir_x, rmode, DIR_W, DIR_E, (delta_g->x != 0));
	get_resize_decor_gravities_one_axis(
		&grav_y, title_dir_y, rmode, DIR_N, DIR_S, (delta_g->y != 0));
	combine_gravities(ret_grav, &grav_x, &grav_y);

	return;
}

/* sets the gravity for the various parts of the window */
static void frame_set_decor_gravities(
	FvwmWindow *fw, frame_decor_gravities_type *grav,
	int do_set1_restore2_bit_gravity)
{
	int valuemask;
	XSetWindowAttributes xcwa;
	int i;
	Bool button_reverted = False;

	/* using bit gravity can reduce redrawing dramatically */
	valuemask = CWWinGravity;
	xcwa.win_gravity = grav->client_grav;
	if (do_set1_restore2_bit_gravity == 1)
	{
		XWindowAttributes xwa;

		if (!fw->attr_backup.is_bit_gravity_stored &&
		    XGetWindowAttributes(dpy, FW_W(fw), &xwa))
		{
			fw->attr_backup.bit_gravity = xwa.bit_gravity;
		}
		fw->attr_backup.is_bit_gravity_stored = 1;
		valuemask |= CWBitGravity;
		xcwa.bit_gravity = grav->client_grav;
	}
	else if (do_set1_restore2_bit_gravity == 2)
	{
		fw->attr_backup.is_bit_gravity_stored = 0;
		valuemask |= CWBitGravity;
		xcwa.bit_gravity = fw->attr_backup.bit_gravity;
	}
	XChangeWindowAttributes(dpy, FW_W(fw), valuemask, &xcwa);
	xcwa.win_gravity = grav->parent_grav;
	valuemask = CWWinGravity;
	XChangeWindowAttributes(dpy, FW_W_PARENT(fw), valuemask, &xcwa);
	if (!HAS_TITLE(fw))
	{
		return;
	}
	xcwa.win_gravity = grav->title_grav;
	XChangeWindowAttributes(dpy, FW_W_TITLE(fw), valuemask, &xcwa);
	if (fw->title_text_rotation == ROTATION_270 ||
	    fw->title_text_rotation == ROTATION_180)
	{
		button_reverted = True;
	}
	if (button_reverted)
	{
		xcwa.win_gravity = grav->rbutton_grav;
	}
	else
	{
		xcwa.win_gravity = grav->lbutton_grav;
	}
	for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i += 2)
	{
		if (FW_W_BUTTON(fw, i))
		{
			XChangeWindowAttributes(
				dpy, FW_W_BUTTON(fw, i), valuemask, &xcwa);
		}
	}
	if (button_reverted)
	{
		xcwa.win_gravity = grav->lbutton_grav;
	}
	else
	{
		xcwa.win_gravity = grav->rbutton_grav;
	}
	for (i = 1; i < NUMBER_OF_TITLE_BUTTONS; i += 2)
	{
		if (FW_W_BUTTON(fw, i))
		{
			XChangeWindowAttributes(
				dpy, FW_W_BUTTON(fw, i), valuemask, &xcwa);
		}
	}

	return;
}

/* Just restore the win and bit gravities on the client window. */
static void frame_restore_client_gravities(FvwmWindow *fw)
{
	XSetWindowAttributes xcwa;
	long valuemask;

	valuemask = CWWinGravity;
	if (fw->attr_backup.is_bit_gravity_stored)
	{
		fw->attr_backup.is_bit_gravity_stored = 0;
		xcwa.bit_gravity = fw->attr_backup.bit_gravity;
		valuemask |= CWBitGravity;
	}
	xcwa.win_gravity = fw->hints.win_gravity;
	XChangeWindowAttributes(dpy, FW_W(fw), valuemask, &xcwa);

	return;
}

/* Prepares the structure for the next animation step. */
static void frame_next_move_resize_args(
	frame_move_resize_args mr_args)
{
	mr_args_internal *mra;

	mra = (mr_args_internal *)mr_args;
	mra->curr_sidebar_g = mra->next_sidebar_g;
	mra->current_g = mra->next_g;
	mra->step_flags.was_hidden = mra->step_flags.is_hidden;
	mra->curr_titlebar_compression = mra->next_titlebar_compression;

	return;
}

static rectangle *frame_get_hidden_pos(
	FvwmWindow *fw, mr_args_internal *mra, Bool do_unhide,
	rectangle *ret_hidden_g)
{
	rectangle *target_g;
	direction_t dir_x;
	direction_t dir_y;

	if (do_unhide == False)
	{
		gravity_split_xy_dir(&dir_x, &dir_y, mra->shade_dir);
		ret_hidden_g->x = (dir_x == DIR_E) ?
		-mra->client_g.width + mra->parent_s.width : 0;
		ret_hidden_g->y = (dir_y == DIR_S) ?
			-mra->client_g.height + mra->parent_s.height : 0;
		target_g = &mra->next_g;
	}
	else
	{
		gravity_split_xy_dir(&dir_x, &dir_y, SHADED_DIR(fw));
		if (mra->mode == FRAME_MR_SCROLL)
		{
			ret_hidden_g->x = (dir_x == DIR_W) ?
				-mra->client_g.width + mra->parent_s.width : 0;
			ret_hidden_g->y = (dir_y == DIR_N) ?
				-mra->client_g.height + mra->parent_s.height :
				0;
		}
		else
		{
			ret_hidden_g->x = (dir_x == DIR_E) ?
				-mra->client_g.width + mra->parent_s.width : 0;
			ret_hidden_g->y = (dir_y == DIR_S) ?
				-mra->client_g.height + mra->parent_s.height :
				0;
		}
		target_g = &mra->next_g;
	}

	return target_g;
}

static void frame_update_hidden_window_pos(
	FvwmWindow *fw, mr_args_internal *mra, Bool do_unhide)
{
	rectangle *target_g;
	rectangle hidden_g;

	target_g = frame_get_hidden_pos(fw, mra, do_unhide, &hidden_g);
	XMoveResizeWindow(
		dpy, FW_W_PARENT(fw), mra->b_g.top_left.width,
		mra->b_g.top_left.height,
		max(1, target_g->width - mra->b_g.total_size.width),
		max(1, target_g->height - mra->b_g.total_size.height));
	XMoveResizeWindow(
		dpy, FW_W(fw), hidden_g.x, hidden_g.y, mra->client_g.width,
		mra->client_g.height);
	mra->flags.was_moved = 1;

	return;
}

static void frame_prepare_animation_shape(
	FvwmWindow *fw, mr_args_internal *mra, int parent_x, int parent_y)
{
	rectangle parent_g;
	rectangle client_g;

	if (!FShapesSupported)
	{
		return;
	}
	parent_g.x = parent_x;
	parent_g.y = parent_y;
	parent_g.width = mra->parent_s.width;
	parent_g.height = mra->parent_s.height;
	gravity_move_resize_parent_child(
		mra->grav.parent_grav, &mra->dstep_g, &parent_g);
	client_g = mra->client_g;
	frame_get_hidden_pos(fw, mra, True, &client_g);
	client_g.x += parent_g.x;
	client_g.y += parent_g.y;
	FShapeCombineShape(
		dpy, Scr.NoFocusWin, FShapeBounding, client_g.x, client_g.y,
		FW_W(fw), FShapeBounding, FShapeSet);
	if (HAS_TITLE(fw))
	{
		rectangle tb_g;
		XRectangle rect;

		SUPPRESS_UNUSED_VAR_WARNING(rect);
		frame_get_titlebar_dimensions_only(
			fw, &mra->next_g, &mra->b_no_title_g, &tb_g);
		/* windows w/ titles */
		rect.x = tb_g.x;
		rect.y = tb_g.y;
		rect.width = tb_g.width;
		rect.height = tb_g.height;
		FShapeCombineRectangles(
			dpy, Scr.NoFocusWin, FShapeBounding, 0, 0, &rect, 1,
			FShapeUnion, Unsorted);
	}

	return;
}

static void frame_mrs_prepare_vars(
	FvwmWindow *fw, mr_args_internal *mra)
{
	Bool dummy;
	int i;

	/* preparations */
	i = mra->current_step;
	mra->next_g = mra->start_g;
	mra->next_g.x += (mra->delta_g.x * i) / mra->anim_steps;
	mra->next_g.y += (mra->delta_g.y * i) / mra->anim_steps;
	mra->next_g.width += (mra->delta_g.width * i) / mra->anim_steps;
	mra->next_g.height += (mra->delta_g.height * i) / mra->anim_steps;
	frame_get_sidebar_geometry(
		fw, NULL, &mra->next_g, &mra->next_sidebar_g, &dummy, &dummy);
	fvwmrect_subtract_rectangles(
		&mra->dstep_g, &mra->next_g, &mra->current_g);
	mra->next_titlebar_compression =
		frame_get_titlebar_compression(fw, &mra->next_g);
	mra->step_flags.is_hidden =
		(frame_is_parent_hidden(fw, &mra->next_g) == True);
	mra->step_flags.do_hide_parent =
		((!mra->step_flags.was_hidden || mra->flags.do_force) &&
		 mra->step_flags.is_hidden);
	mra->step_flags.do_unhide_parent =
		((mra->step_flags.was_hidden || mra->flags.do_force) &&
		 !mra->step_flags.is_hidden);
	/* get the parent's dimensions */
	mra->parent_s.width = mra->next_g.width - mra->b_g.total_size.width;
	if (mra->parent_s.width < 1)
	{
		mra->minimal_w_offset = 1 - mra->parent_s.width;
		mra->parent_s.width = 1;
	}
	else
	{
		mra->minimal_w_offset = 0;
	}
	mra->parent_s.height = mra->next_g.height - mra->b_g.total_size.height;
	if (mra->parent_s.height < 1)
	{
		mra->minimal_h_offset = 1 - mra->parent_s.height;
		mra->parent_s.height = 1;
	}
	else
	{
		mra->minimal_h_offset = 0;
	}

	return;
}

static void frame_mrs_hide_changing_parts(
	mr_args_internal *mra)
{
	int l_add;
	int t_add;
	int r_add;
	int b_add;
	int w;
	int h;

	t_add = 0;
	l_add = 0;
	b_add = 0;
	r_add = 0;
	if (mra->mode == FRAME_MR_SHRINK)
	{
		if (mra->dstep_g.x > 0)
		{
			l_add = mra->dstep_g.x;
		}
		if (mra->dstep_g.y > 0)
		{
			t_add = mra->dstep_g.y;
		}
	}
	else if (mra->mode == FRAME_MR_SCROLL)
	{
		if (mra->dstep_g.x == 0 && mra->dstep_g.width < 0)
		{
			l_add = -mra->dstep_g.width;
		}
		if (mra->dstep_g.y == 0 && mra->dstep_g.height < 0)
		{
			t_add = -mra->dstep_g.height;
		}
	}
	if (l_add > 0)
	{
		l_add -= mra->minimal_w_offset;
	}
	else
	{
		r_add = (mra->dstep_g.width < 0) ? -mra->dstep_g.width : 0;
		r_add -= mra->minimal_w_offset;
	}
	if (t_add > 0)
	{
		t_add -= mra->minimal_h_offset;
	}
	else
	{
		b_add = (mra->dstep_g.height < 0) ? -mra->dstep_g.height : 0;
		b_add -= mra->minimal_h_offset;
	}
	/* cover top border */
	w = mra->current_g.width;
	h = mra->b_g.top_left.height + t_add;
	if (w > 0 && h > 0)
	{
		XMoveResizeWindow(dpy, hide_wins.w[0], 0, 0, w, h);
		XMapWindow(dpy, hide_wins.w[0]);
	}
	/* cover left border */
	w = mra->b_g.top_left.width + l_add;
	h = mra->current_g.height;
	if (w > 0 && h > 0)
	{
		XMoveResizeWindow(dpy, hide_wins.w[1], 0, 0, w, h);
		XMapWindow(dpy, hide_wins.w[1]);
	}
	/* cover bottom border and possibly part of the client */
	w = mra->current_g.width;
	h = mra->b_g.bottom_right.height + b_add;
	if (w > 0 && h > 0)
	{
		XMoveResizeWindow(
			dpy, hide_wins.w[2], 0, mra->current_g.height -
			mra->b_g.bottom_right.height - b_add, w, h);
		XMapWindow(dpy, hide_wins.w[2]);
	}
	/* cover right border and possibly part of the client */
	w = mra->b_g.bottom_right.width + r_add;
	h = mra->current_g.height;
	if (w > 0 && h > 0)
	{
		XMoveResizeWindow(
			dpy, hide_wins.w[3], mra->current_g.width -
			mra->b_g.bottom_right.width - r_add, 0, w, h);
		XMapWindow(dpy, hide_wins.w[3]);
	}

	return;
}

static void frame_mrs_hide_unhide_parent(
	FvwmWindow *fw, mr_args_internal *mra)
{
	XSetWindowAttributes xswa;

	if (mra->step_flags.do_unhide_parent)
	{
		Window w[2];

		/* update the hidden position of the client */
		frame_update_hidden_window_pos(fw, mra, True);
		w[0] = hide_wins.w[3];
		w[1] = FW_W_PARENT(fw);
		XRestackWindows(dpy, w, 2);
	}
	else if (mra->step_flags.do_hide_parent)
	{
		/* When the parent gets hidden, unmap it automatically, lower
		 * it while hidden, then remap it.  Necessary to eliminate
		 * flickering. */
		xswa.win_gravity = UnmapGravity;
		XChangeWindowAttributes(
			dpy, FW_W_PARENT(fw), CWWinGravity, &xswa);
	}

	return;
}

static void frame_mrs_hide_unhide_parent2(
	FvwmWindow *fw, mr_args_internal *mra)
{
	XSetWindowAttributes xswa;

	/* finish hiding the parent */
	if (mra->step_flags.do_hide_parent)
	{
		xswa.win_gravity = mra->grav.parent_grav;
		XChangeWindowAttributes(
			dpy, FW_W_PARENT(fw), CWWinGravity, &xswa);
		/* update the hidden position of the client */
		frame_update_hidden_window_pos(fw, mra, False);
		XLowerWindow(dpy, FW_W_PARENT(fw));
		XMapWindow(dpy, FW_W_PARENT(fw));
	}

	return;
}

static void frame_mrs_setup_draw_decorations(
	FvwmWindow *fw, mr_args_internal *mra)
{
	window_parts setup_parts;

	/* setup the title bar and the border */
	setup_parts = PART_TITLE;
	if (mra->curr_titlebar_compression != mra->next_titlebar_compression ||
	    mra->mode == FRAME_MR_FORCE_SETUP)
	{
		setup_parts |= PART_BUTTONS;
	}
	frame_setup_titlebar(fw, &mra->next_g, setup_parts, &mra->dstep_g);
	frame_setup_border(fw, &mra->next_g, PART_ALL, &mra->dstep_g);
	/* draw the border and the titlebar */
	if (mra->flags.do_not_draw == 1)
	{
		/* nothing */
	}
	else if (!mra->flags.is_shading || !mra->flags.is_lazy_shading)
	{
		/* draw as usual */
		border_draw_decorations(
			fw, PART_ALL,
			(mra->w_with_focus != None) ? True : False,
			(mra->flags.do_force) ? True : False, CLEAR_ALL,
			&mra->current_g, &mra->next_g);
	}
	else /* lazy shading */
	{
		/* Lazy shading relies on the proper window gravities and does
		 * not redraw the decorations during the animation.  Only draw
		 * on the first step. */
		if (mra->current_step == 1)
		{
			rectangle lazy_g;

			/* Use the larger width and height values from the
			 * current and the final geometry to get proper
			 * decorations. */
			lazy_g.x = mra->current_g.x;
			lazy_g.y = mra->current_g.y;
			lazy_g.width = max(
				mra->current_g.width, mra->end_g.width);
			lazy_g.height = max(
				mra->current_g.height, mra->end_g.height);
			border_draw_decorations(
				fw, PART_ALL,
				(mra->w_with_focus != None) ? True : False,
				True, CLEAR_ALL, &mra->current_g, &lazy_g);
		}
	}

	return;
}

static void frame_mrs_resize_move_windows(
	FvwmWindow *fw, mr_args_internal *mra)
{
	/* setup the parent, the frame and the client window */
	if (!mra->flags.is_shading)
	{
		if (!mra->step_flags.is_hidden &&
		    !mra->flags.do_not_configure_client)
		{
			XMoveResizeWindow(
				dpy, FW_W(fw), 0, 0, mra->parent_s.width,
				mra->parent_s.height);
			mra->client_g.width = mra->parent_s.width;
			mra->client_g.height = mra->parent_s.height;
		}
		else
		{
			XMoveWindow(dpy, FW_W(fw), 0, 0);
		}
		mra->flags.was_moved = 1;
#if 0
		/* reduces flickering */
		/* dv (11-Aug-2002): ... and slows down large scripts
		 * dramatically.  Rather let it flicker */
		if (mra->flags.is_setup)
		{
			usleep(1000);
		}
#endif
		XSync(dpy, 0);
		XMoveResizeWindow(
			dpy, FW_W_PARENT(fw), mra->b_g.top_left.width,
			mra->b_g.top_left.height, mra->parent_s.width,
			mra->parent_s.height);
	}
	else
	{
		int x;
		int y;

		x = mra->b_g.top_left.width;
		y = mra->b_g.top_left.height;
		if (mra->mode == FRAME_MR_SCROLL)
		{
			if (mra->dstep_g.x == 0)
			{
				x -= mra->dstep_g.width -
					mra->minimal_w_offset;
			}
			if (mra->dstep_g.y == 0)
			{
				y -= mra->dstep_g.height -
					mra->minimal_h_offset;;
			}
		}
		else if (mra->mode == FRAME_MR_SHRINK)
		{
			x += mra->dstep_g.x;
			y += mra->dstep_g.y;
		}
		if (x > 0)
		{
			x -= mra->minimal_w_offset;
		}
		else if (x < 0)
		{
			x += mra->minimal_w_offset;
		}
		if (y > 0)
		{
			y -= mra->minimal_h_offset;
		}
		else if (y < 0)
		{
			y += mra->minimal_h_offset;
		}
		XMoveResizeWindow(
			dpy, FW_W_PARENT(fw), x, y, mra->parent_s.width,
			mra->parent_s.height);
		if (mra->flags.do_update_shape)
		{
			/* Step 1: apply the union of the old and new shapes.
			 * This way so that the client stays visible - rather
			 * let the background show through than force an
			 * Expose event. */
			frame_prepare_animation_shape(fw, mra, x, y);
			FShapeCombineShape(
				dpy, FW_W_FRAME(fw), FShapeBounding, 0, 0,
				Scr.NoFocusWin, FShapeBounding, FShapeUnion);
		}
	}
	XMoveResizeWindow(
		dpy, FW_W_FRAME(fw), mra->next_g.x, mra->next_g.y,
		mra->next_g.width, mra->next_g.height);
	if (mra->flags.do_update_shape)
	{
		/* Step 2: clip the previous shape. */
		FShapeCombineShape(
			dpy, FW_W_FRAME(fw), FShapeBounding, 0, 0,
			Scr.NoFocusWin, FShapeBounding, FShapeSet);
	}

	return;
}

static void frame_move_resize_step(
	FvwmWindow *fw, mr_args_internal *mra)
{
	frame_mrs_prepare_vars(fw, mra);
	frame_mrs_hide_changing_parts(mra);
	frame_mrs_hide_unhide_parent(fw, mra);
	frame_mrs_setup_draw_decorations(fw, mra);
	frame_mrs_resize_move_windows(fw, mra);
	frame_mrs_hide_unhide_parent2(fw, mra);
	fw->g.frame = mra->next_g;

	return;
}


static void frame_has_handles_and_tiled_border(
	FvwmWindow *fw, int *ret_has_handles, int *ret_has_tiled_border)
{
	DecorFace *df;

	*ret_has_handles = 1;
	if (!HAS_HANDLES(fw))
	{
		*ret_has_handles = 0;
	}
	df = border_get_border_style(fw, (fw == Scr.Hilite) ? True : False);
	if (DFS_HAS_HIDDEN_HANDLES(df->style))
	{
		*ret_has_handles = 0;
	}
	*ret_has_tiled_border =
		(DFS_FACE_TYPE(df->style) == TiledPixmapButton) ||
		(DFS_FACE_TYPE(df->style) == ColorsetButton &&
		 !CSET_IS_TRANSPARENT_PR(df->u.acs.cs) &&
		 CSET_HAS_PIXMAP(df->u.acs.cs));

	return;
}

static int frame_get_shading_laziness(
	FvwmWindow *fw, mr_args_internal *mra)
{
	int has_handles;
	int has_tiled_pixmap_border;
	int using_border_style;

	if (!mra->flags.is_shading || mra->anim_steps <= 2)
	{
		return 0;
	}
	frame_has_handles_and_tiled_border(
		fw, &has_handles, &has_tiled_pixmap_border);
	if (HAS_TITLE(fw) && has_tiled_pixmap_border)
	{
		using_border_style = border_is_using_border_style(
			fw, (fw == Scr.Hilite) ? True : False);
	}
	else
	{
		using_border_style = 0;
	}
	switch (WINDOWSHADE_LAZINESS(fw))
	{
	case WINDOWSHADE_ALWAYS_LAZY:
		return 1;
	case WINDOWSHADE_BUSY:
		if (has_handles)
		{
			return 0;
		}
		/* fall through */
	case WINDOWSHADE_LAZY:
	default:
		if (has_tiled_pixmap_border && !HAS_NO_BORDER(fw))
		{
			return 0;
		}
		else if (has_tiled_pixmap_border && HAS_TITLE(fw) &&
			 using_border_style)
		{
			return 0;
		}
		return 1;
	}
}

void frame_reshape_border(FvwmWindow *fw)
{
	int grav;
	int off_x = 0;
	int off_y = 0;
	rectangle naked_g;
	rectangle *new_g;

	/* calculate the new offsets */
	if (!IS_MAXIMIZED(fw))
	{
		grav = fw->hints.win_gravity;
		new_g = &fw->g.normal;
	}
	else
	{
		/* maximized windows are always considered to have
		 * NorthWestGravity */
		grav = NorthWestGravity;
		new_g = &fw->g.max;
		off_x = fw->g.normal.x - fw->g.max.x;
		off_y = fw->g.normal.y - fw->g.max.y;
	}
	gravity_get_naked_geometry(grav, fw, &naked_g, new_g);
	gravity_translate_to_northwest_geometry_no_bw(
		grav, fw, &naked_g, &naked_g);
	set_window_border_size(fw, fw->unshaped_boundary_width);
	gravity_add_decoration(grav, fw, new_g, &naked_g);
	if (IS_MAXIMIZED(fw))
	{
		/* prevent random paging when unmaximizing after the border
		 * width has changed */
		fw->g.max_offset.x += fw->g.normal.x - fw->g.max.x - off_x;
		fw->g.max_offset.y += fw->g.normal.y - fw->g.max.y - off_y;
	}
	if (IS_SHADED(fw))
	{
		get_unshaded_geometry(fw, new_g);
		if (USED_TITLE_DIR_FOR_SHADING(fw))
		{
			SET_SHADED_DIR(fw, GET_TITLE_DIR(fw));
		}
		get_shaded_geometry(fw, &fw->g.frame, new_g);
		frame_force_setup_window(
			fw, fw->g.frame.x, fw->g.frame.y, fw->g.frame.width,
			fw->g.frame.height, False);
	}
	else
	{
		get_relative_geometry(new_g, new_g);
		frame_force_setup_window(
			fw, new_g->x, new_g->y, new_g->width, new_g->height,
			True);
	}

	return;
}

/* ---------------------------- interface functions ------------------------ */

/* Initialise structures local to frame.c */
void frame_init(void)
{
	XSetWindowAttributes xswa;
	unsigned long valuemask;
	int i;

	xswa.override_redirect = True;
	xswa.backing_store = NotUseful;
	xswa.save_under = False;
	xswa.win_gravity = UnmapGravity;
	xswa.background_pixmap = None;
	valuemask = CWOverrideRedirect | CWSaveUnder | CWBackingStore |
		CWBackPixmap | CWWinGravity;
	hide_wins.parent = Scr.Root;
	for (i = 0; i < 4; i++)
	{
		hide_wins.w[i] = XCreateWindow(
			dpy, Scr.Root, -1, -1, 1, 1, 0, CopyFromParent,
			InputOutput, CopyFromParent, valuemask, &xswa);
		if (hide_wins.w[i] == None)
		{
			fvwm_msg(ERR, "frame_init",
				 "Could not create internal windows. Exiting");
			MyXUngrabServer(dpy);
			exit(1);
		}
	}

	return;
}

Bool is_frame_hide_window(
	Window w)
{
	int i;

	if (w == None)
	{
		return False;
	}
	for (i = 0; i < 4; i++)
	{
		if (w == hide_wins.w[i])
		{
			return True;
		}
	}

	return False;
}

void frame_destroyed_frame(
	Window frame_w)
{
	if (hide_wins.parent == frame_w)
	{
		/* Oops, the window containing the hide windows was destroyed!
		 * Let it die and create them from scratch. */
		frame_init();
	}

	return;
}

void frame_get_titlebar_dimensions(
	FvwmWindow *fw, rectangle *frame_g, rectangle *diff_g,
	frame_title_layout_t *title_layout)
{
	size_borders b;
	int i;
	int tb_length;
	int tb_thick;
	int tb_x;
	int tb_y;
	int b_length;
	int b_w;
	int b_h;
	int t_length;
	int t_w;
	int t_h;
	int br_sub;
	int nbuttons;
	int nbuttons_big;
	int *padd_coord;
	int *b_l;
	Bool revert_button = False;

	if (!HAS_TITLE(fw))
	{
		return;
	}
	get_window_borders_no_title(fw, &b);
	if (HAS_VERTICAL_TITLE(fw))
	{
		tb_length = frame_g->height - b.total_size.height;
	}
	else
	{
		tb_length = frame_g->width - b.total_size.width;
	}
	/* find out the length of the title and the buttons */
	tb_thick = fw->title_thickness;
	nbuttons = fw->nr_left_buttons + fw->nr_right_buttons;
	nbuttons_big = 0;
	b_length = tb_thick;
	t_length = tb_length - nbuttons * b_length;
	if (nbuttons > 0 && t_length < MIN_WINDOW_TITLE_LENGTH)
	{
		int diff = MIN_WINDOW_TITLE_LENGTH - t_length;
		int pixels = diff / nbuttons;

		b_length -= pixels;
		t_length += nbuttons * pixels;
		nbuttons_big = nbuttons - (MIN_WINDOW_TITLE_LENGTH - t_length);
		t_length = MIN_WINDOW_TITLE_LENGTH;
	}
	if (b_length < MIN_WINDOW_TITLEBUTTON_LENGTH)
	{
		/* don't draw the buttons */
		nbuttons = 0;
		nbuttons_big = 0;
		b_length = 0;
		t_length = tb_length;
	}
	if (t_length < 0)
	{
		t_length = 0;
	}
	fw->title_length = t_length;
	/* prepare variables */
	if (HAS_VERTICAL_TITLE(fw))
	{
		tb_y = b.top_left.height;
		br_sub = (diff_g != NULL) ? diff_g->height : 0;
		if (GET_TITLE_DIR(fw) == DIR_W)
		{
			tb_x = b.top_left.width;
		}
		else
		{
			tb_x = frame_g->width - b.bottom_right.width -
				tb_thick;
			if (diff_g != NULL)
			{
				tb_x -= diff_g->width;
			}
		}
		padd_coord = &tb_y;
		b_w = tb_thick;
		b_h = b_length;
		t_w = tb_thick;
		t_h = t_length;
		b_l = &b_h;
	}
	else
	{
		tb_x = b.top_left.width;
		br_sub = (diff_g != NULL) ? diff_g->width : 0;
		if (GET_TITLE_DIR(fw) == DIR_N)
		{
			tb_y = b.top_left.height;
		}
		else
		{
			tb_y = frame_g->height - b.bottom_right.height -
				tb_thick;
			if (diff_g != NULL)
			{
				tb_y -= diff_g->height;
			}
		}
		padd_coord = &tb_x;
		b_w = b_length;
		b_h = tb_thick;
		t_w = t_length;
		t_h = tb_thick;
		b_l = &b_w;
	}
	if (fw->title_text_rotation == ROTATION_270 ||
	    fw->title_text_rotation == ROTATION_180)
	{
		revert_button = True;
	}

	/* configure left buttons */
	for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i++)
	{
		if ((revert_button && !(i & 1)) ||
		    (!revert_button && (i & 1)) || FW_W_BUTTON(fw, i) == None)
		{
			continue;
		}
		if (b_length <= 0)
		{
			title_layout->button_g[i].x = -1;
			title_layout->button_g[i].y = -1;
			title_layout->button_g[i].width = 1;
			title_layout->button_g[i].height = 1;
		}
		else
		{
			title_layout->button_g[i].x = tb_x;
			title_layout->button_g[i].y = tb_y;
			title_layout->button_g[i].width = b_w;
			title_layout->button_g[i].height = b_h;
		}
		*padd_coord += b_length;
		nbuttons_big--;
		if (nbuttons_big == 0)
		{
			b_length--;
			(*b_l)--;
		}
	}
	/* configure title */
	if (t_length == 0)
	{
		title_layout->title_g.x = -1;
		title_layout->title_g.y = -1;
		title_layout->title_g.width = 1;
		title_layout->title_g.height = 1;
	}
	else
	{
		title_layout->title_g.x = tb_x;
		title_layout->title_g.y = tb_y;
		title_layout->title_g.width = t_w;
		title_layout->title_g.height = t_h;
	}
	*padd_coord += t_length;
	/* configure right buttons */
	*padd_coord -= br_sub;
	for (i = NUMBER_OF_TITLE_BUTTONS-1; i > -1; i--)
	{
		if ((revert_button && (i & 1)) ||
		    (!revert_button && !(i & 1)) || FW_W_BUTTON(fw, i) == None)
		{
			continue;
		}
		if (b_length <= 0)
		{
			title_layout->button_g[i].x = -1;
			title_layout->button_g[i].y = -1;
			title_layout->button_g[i].width = 1;
			title_layout->button_g[i].height = 1;
		}
		else
		{
			title_layout->button_g[i].x = tb_x;
			title_layout->button_g[i].y = tb_y;
			title_layout->button_g[i].width = b_w;
			title_layout->button_g[i].height = b_h;
		}
		*padd_coord += b_length;
		nbuttons_big--;
		if (nbuttons_big == 0)
		{
			b_length--;
			(*b_l)--;
		}
	}

	return;
}

void frame_get_sidebar_geometry(
	FvwmWindow *fw, DecorFaceStyle *borderstyle, rectangle *frame_g,
	rectangle *ret_g, Bool *ret_has_x_marks, Bool *ret_has_y_marks)
{
	int min_w;
	size_borders b;

	ret_g->x = 0;
	ret_g->y = 0;
	ret_g->width = 0;
	ret_g->height = 0;
	*ret_has_x_marks = False;
	*ret_has_y_marks = False;
	if (HAS_NO_BORDER(fw))
	{
		return;
	}
	/* get the corner size */
	if (!HAS_HANDLES(fw))
	{
		*ret_has_x_marks = False;
		*ret_has_y_marks = False;
	}
	else if (borderstyle == NULL)
	{
		if (fw->decor_state.parts_drawn & PART_X_HANDLES)
		{
			*ret_has_x_marks = True;
		}
		if (fw->decor_state.parts_drawn & PART_Y_HANDLES)
		{
			*ret_has_y_marks = True;
		}
	}
	else if (!DFS_HAS_HIDDEN_HANDLES(*borderstyle))
	{
		*ret_has_x_marks = True;
		*ret_has_y_marks = True;
	}
	ret_g->x = fw->corner_width;
	ret_g->y = fw->corner_width;
	min_w = 2 * fw->corner_width + 4;
	/* limit by available space, remove handle marks if necessary */
	if (frame_g->width < min_w)
	{
		ret_g->x = frame_g->width / 3;
		*ret_has_y_marks = False;
	}
	if (frame_g->height < min_w)
	{
		ret_g->y = frame_g->height / 3;
		*ret_has_x_marks = False;
	}
	get_window_borders_no_title(fw, &b);
	if (ret_g->x < b.top_left.width)
	{
		ret_g->x = b.top_left.width;
	}
	if (ret_g->y < b.top_left.height)
	{
		ret_g->y = b.top_left.height;
	}
	/* length of the side bars */
	ret_g->width = frame_g->width - 2 * ret_g->x;
	ret_g->height = frame_g->height - 2 * ret_g->y;

	return;
}

int frame_window_id_to_context(
	FvwmWindow *fw, Window w, int *ret_num)
{
	int context = C_ROOT;

	*ret_num = -1;
	if (fw == NULL || w == None)
	{
		return C_ROOT;
	}
	if (w == FW_W_TITLE(fw))
	{
		context = C_TITLE;
	}
	else if (Scr.EwmhDesktop &&
		 (w == FW_W(Scr.EwmhDesktop) ||
		  w == FW_W_PARENT(Scr.EwmhDesktop) ||
		  w == FW_W_FRAME(Scr.EwmhDesktop)))
	{
		context = C_EWMH_DESKTOP;
	}
	else if (w == FW_W(fw) || w == FW_W_PARENT(fw) || w == FW_W_FRAME(fw))
	{
		context = C_WINDOW;
	}
	else if (w == FW_W_ICON_TITLE(fw) || w == FW_W_ICON_PIXMAP(fw))
	{
		context = C_ICON;
	}
	else if (w == FW_W_CORNER(fw, 0))
	{
		*ret_num = 0;
		context = C_F_TOPLEFT;
	}
	else if (w == FW_W_CORNER(fw, 1))
	{
		*ret_num = 1;
		context = C_F_TOPRIGHT;
	}
	else if (w == FW_W_CORNER(fw, 2))
	{
		*ret_num = 2;
		context = C_F_BOTTOMLEFT;
	}
	else if (w == FW_W_CORNER(fw, 3))
	{
		*ret_num = 3;
		context = C_F_BOTTOMRIGHT;
	}
	else if (w == FW_W_SIDE(fw, 0))
	{
		*ret_num = 0;
		context = C_SB_TOP;
	}
	else if (w == FW_W_SIDE(fw, 1))
	{
		*ret_num = 1;
		context = C_SB_RIGHT;
	}
	else if (w == FW_W_SIDE(fw, 2))
	{
		*ret_num = 2;
		context = C_SB_BOTTOM;
	}
	else if (w == FW_W_SIDE(fw, 3))
	{
		*ret_num = 3;
		context = C_SB_LEFT;
	}
	else
	{
		int i;

		for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i++)
		{
			if (w == FW_W_BUTTON(fw, i) &&
			    ((!(i & 1) && i / 2 < Scr.nr_left_buttons) ||
			     ( (i & 1) && i / 2 < Scr.nr_right_buttons)))
			{
				context = (1 << i) * C_L1;
				*ret_num = i;
				break;
			}
		}
	}
	if (!HAS_HANDLES(fw) && (context & (C_SIDEBAR | C_FRAME)))
	{
		context = C_SIDEBAR;
	}

	return context;
}

/* Creates a structure that must be passed to frame_move_resize().  The
 * structure *must* be deleted with frame_free_move_resize_args() as soon as the
 * move or resize operation is finished.  Prepares the window for a move/resize
 * operation.
 *
 * Arguments:
 *   fw
 *     The window to move or resize.
 *   mr_mode
 *     The mode of operation:
 *       FRAME_MR_SETUP: setup the frame
 *       FRAME_MR_FORCE_SETUP: same, but forces all updates
 *       FRAME_MR_OPAQUE: resize the frame in an opaque fashion
 *       FRAME_MR_SHRINK: shrink the client window (useful for shading only)
 *       FRAME_MR_SCROLL: scroll the client window (useful for shading only)
 *   start_g
 *     The initial geometry of the frame.  If a NULL pointer is passed, the
 *     frame_g member of the window is used instead.
 *   end_g
 *     The desired new geometry of the frame.
 *   anim_steps
 *     The number of animation steps in between
 *       = 0: The operation is finished in a single step.
 *       > 0: The given number of steps are drawn in between.
 *       < 0: Each step resizes the window by the given number of pixels.
 *            (the sign of the number is flipped first).
 *     This argument is used only with FRAME_MR_SHRINK and FRAME_MR_SCROLL.
 */
frame_move_resize_args frame_create_move_resize_args(
	FvwmWindow *fw, frame_move_resize_mode mr_mode,
	rectangle *start_g, rectangle *end_g, int anim_steps, int shade_dir)
{
	mr_args_internal *mra;
	Bool dummy;
	Bool rc;
	int whdiff;
	int xydiff;
	int diff;

	/* set some variables */
	mra = xcalloc(1, sizeof(mr_args_internal));
	if (mr_mode & FRAME_MR_DONT_DRAW)
	{
		mr_mode &= ~FRAME_MR_DONT_DRAW;
		mra->flags.do_not_draw = 1;
	}
	else
	{
		mra->flags.do_not_draw = 0;
	}
	if (mr_mode == FRAME_MR_SETUP_BY_APP)
	{
		mr_mode = FRAME_MR_SETUP;
		mra->flags.do_set_bit_gravity = 0;
	}
	else
	{
		mra->flags.do_set_bit_gravity = 1;
	}
	if (mr_mode == FRAME_MR_FORCE_SETUP_NO_W)
	{
		mr_mode = FRAME_MR_FORCE_SETUP;
		mra->flags.do_not_configure_client = 1;
	}
	mra->mode = mr_mode;
	mra->shade_dir = (direction_t)shade_dir;
	mra->w_with_focus = (fw == get_focus_window()) ? FW_W(fw) : None;

	/* calculate various geometries */
	if (!IS_SHADED(fw))
	{
		rc = XGetGeometry(
			dpy, FW_W(fw), &JunkRoot, &mra->client_g.x,
			&mra->client_g.y, (unsigned int*)&mra->client_g.width,
			(unsigned int*)&mra->client_g.height,
			(unsigned int*)&JunkBW,	(unsigned int*)&JunkDepth);
		if (rc == True)
		{
			rc = XTranslateCoordinates(
				dpy, FW_W_PARENT(fw), Scr.Root,
				mra->client_g.x, mra->client_g.y,
				&mra->client_g.x, &mra->client_g.y,
				&JunkChild);
		}
		if (rc == False)
		{
			/* Can only happen if the window died */
			get_client_geometry(fw, &mra->client_g);
		}
	}
	else
	{
		/* If the window was reisez while shaded the client window
		 * will not have been resized. Use the frame to get the
		 * geometry */
		get_client_geometry(fw, &mra->client_g);
	}
	get_window_borders(fw, &mra->b_g);
	get_window_borders_no_title(fw, &mra->b_no_title_g);
	mra->start_g = (start_g != NULL) ? *start_g : fw->g.frame;
	frame_get_sidebar_geometry(
		fw, NULL, &mra->start_g, &mra->curr_sidebar_g, &dummy, &dummy);
	mra->end_g = *end_g;
	mra->current_g = mra->start_g;
	mra->next_g = mra->end_g;
	mra->curr_titlebar_compression =
		frame_get_titlebar_compression(fw, &mra->start_g);
	fvwmrect_subtract_rectangles(
		&mra->delta_g, &mra->end_g, &mra->start_g);

	/* calcuate the number of animation steps */
	switch (mra->mode)
	{
	case FRAME_MR_SHRINK:
	case FRAME_MR_SCROLL:
		whdiff = max(abs(mra->delta_g.width), abs(mra->delta_g.height));
		xydiff = max(abs(mra->delta_g.x), abs(mra->delta_g.y));
		diff = max(whdiff, xydiff);
		if (diff == 0)
		{
			mra->anim_steps = 0;
		}
		else if (anim_steps < 0)
		{
			mra->anim_steps = -(diff - 1) / anim_steps;
		}
		else if (anim_steps > 0 && anim_steps >= diff)
		{
			mra->anim_steps = diff - 1;
		}
		else
		{
			mra->anim_steps = anim_steps;
		}
		mra->anim_steps++;
		break;
	case FRAME_MR_FORCE_SETUP:
	case FRAME_MR_SETUP:
	case FRAME_MR_OPAQUE:
	default:
		mra->anim_steps = 1;
		break;
	}

	/* various flags */
	mra->flags.was_moved = 0;
	mra->step_flags.was_hidden =
		(frame_is_parent_hidden(fw, &mra->start_g) == True);
	mra->flags.is_setup =
		(mra->mode == FRAME_MR_FORCE_SETUP ||
		 mra->mode == FRAME_MR_SETUP);
	mra->flags.do_force = (mra->mode == FRAME_MR_FORCE_SETUP);
	mra->flags.is_shading =
		!(mra->flags.is_setup || mra->mode == FRAME_MR_OPAQUE);
	mra->flags.do_update_shape =
		(FShapesSupported && mra->flags.is_shading && fw->wShaped);
	/* Lazy shading does not draw the hadle marks.  Disable them in the
	 * window flags if necessary.  Restores the marks when mr_args are
	 * freed.  Lazy shading is considerably faster but causes funny looks
	 * if either the border uses a tiled pixmap background. */
	mra->flags.had_handles = HAS_HANDLES(fw);
	mra->flags.is_lazy_shading = frame_get_shading_laziness(fw, mra);
	mra->trans_parts = border_get_transparent_decorations_part(fw);
	if (mra->flags.is_lazy_shading)
	{
		SET_HAS_HANDLES(fw, 0);
	}

	/* Set gravities for the window parts. */
	frame_get_resize_decor_gravities(
		&mra->grav, GET_TITLE_DIR(fw), mra->mode, &mra->delta_g);
	if (mra->flags.is_setup && mra->delta_g.x == 0 && mra->delta_g.y == 0 &&
	    mra->delta_g.width == 0 && mra->delta_g.height == 0)
	{
		frame_decor_gravities_type grav;

		/* The caller has already set the frame geometry.  Use
		 * StaticGravity so the sub windows are not moved to funny
		 * places. */
		grav.decor_grav = StaticGravity;
		grav.title_grav = StaticGravity;
		grav.lbutton_grav = StaticGravity;
		grav.rbutton_grav = StaticGravity;
		grav.parent_grav = StaticGravity;
		grav.client_grav = StaticGravity;
		frame_set_decor_gravities(
			fw, &grav, (mra->flags.do_set_bit_gravity) ? 1 : 0);
		mra->flags.do_restore_gravity = 1;
		mra->flags.do_force = 1;
	}
	else
	{
		frame_set_decor_gravities(
			fw, &mra->grav,
			(mra->flags.do_set_bit_gravity) ? 1 : 0);
	}
	frame_reparent_hide_windows(FW_W_FRAME(fw));

	return (frame_move_resize_args)mra;
}

/* Changes the final_geometry in a frame_move_resize_args structure.  This is
 * useful during opaque resize operations to avoid creating and destroying the
 * args for each animation step.
 *
 * If FRAME_MR_SHRINK or  FRAME_MR_SCROLL was used to greate the mr_args, the
 * function does nothing.
 */
void frame_update_move_resize_args(
	frame_move_resize_args mr_args, rectangle *end_g)
{
	mr_args_internal *mra;

	mra = (mr_args_internal *)mr_args;
	mra->end_g = *end_g;
	fvwmrect_subtract_rectangles(
		&mra->delta_g, &mra->end_g, &mra->start_g);

	return;
}

/* Destroys the structure allocated with frame_create_move_resize_args().  Does
 * some clean up operations on the modified window first. */
void frame_free_move_resize_args(
	FvwmWindow *fw, frame_move_resize_args mr_args)
{
	mr_args_internal *mra;

	mra = (mr_args_internal *)mr_args;
	SET_HAS_HANDLES(fw, mra->flags.had_handles);
	fw->g.frame = mra->end_g;
	if (mra->flags.is_lazy_shading)
	{
		border_draw_decorations(
			fw, PART_ALL,
			(mra->w_with_focus != None) ? True : False, True,
			CLEAR_ALL, &mra->current_g, &mra->end_g);
	}
	else if (mra->trans_parts != PART_NONE)
	{
		border_draw_decorations(
			fw, mra->trans_parts,
			(mra->w_with_focus != None) ? True : False, True,
			CLEAR_ALL, &mra->current_g, &mra->end_g);
	}
	update_absolute_geometry(fw);
	frame_reparent_hide_windows(Scr.NoFocusWin);
	if (mra->w_with_focus != None && FP_IS_LENIENT(FW_FOCUS_POLICY(fw)))
	{
		/* domivogt (28-Dec-1999): For some reason the XMoveResize() on
		 * the frame window removes the input focus from the client
		 * window.  I have no idea why, but if we explicitly restore
		 * the focus here everything works fine. */
		FOCUS_SET(mra->w_with_focus, fw);
	}
	if (mra->flags.do_update_shape)
	{
		/* unset shape */
		FShapeCombineMask(
			dpy, FW_W_FRAME(fw), FShapeBounding, 0, 0, None,
			FShapeSet);
	}
	frame_setup_shape(fw, mra->end_g.width, mra->end_g.height, fw->wShaped);
	if (mra->flags.do_restore_gravity)
	{
		/* TA:  2011-09-04: There might be a chance some clients with
		 *      a gravity other than Static (such as non-NW gravity)
		 *      might not react well -- but setting the gravity to the
		 *      main window hint will break clients being remapped as
		 *      subwindows, c.f. XEmbed.
		 *
		 *      Note that we should probably consider handling
		 *      GravityNotify events ourselves, since we already set
		 *      StructureNotify and SubstructureNotify events on
		 *      FW_W_PARENT for example.
		 *
		 * mra->grav.client_grav = fw->hints.win_gravity;
		 */
		frame_set_decor_gravities(
			fw, &mra->grav,
			(mra->flags.do_set_bit_gravity) ? 2 : 0);
	}
	else
	{
		frame_restore_client_gravities(fw);
	}
	if (!IS_SHADED(fw) && mra->flags.was_moved)
	{
		SendConfigureNotify(
			fw, fw->g.frame.x, fw->g.frame.y, fw->g.frame.width,
			fw->g.frame.height, 0, True);
		mra->flags.was_moved = 0;
	}
	focus_grab_buttons_on_layer(fw->layer);
	/* free the memory */
	free(mr_args);

	return;
}

void frame_move_resize(
	FvwmWindow *fw, frame_move_resize_args mr_args)
{
	mr_args_internal *mra;
	Bool is_grabbed = False;
	int i;

	mra = (mr_args_internal *)mr_args;
	/* freeze the cursor shape; otherwise it may flash to a different shape
	 * during the animation */
	if (mra->anim_steps > 1)
	{
		is_grabbed = GrabEm(None, GRAB_FREEZE_CURSOR);
	}
	/* animation */
	for (i = 1; i <= mra->anim_steps; i++, frame_next_move_resize_args(mra))
	{
		mra->current_step = i;
		frame_move_resize_step(fw, mra);
	}
	if (is_grabbed == True)
	{
		UngrabEm(GRAB_FREEZE_CURSOR);
	}

	return;
}

/***********************************************************************
 *
 *  Procedure:
 *      frame_setup_window - set window sizes
 *
 *  Inputs:
 *      fw - the FvwmWindow pointer
 *      x       - the x coordinate of the upper-left outer corner of the frame
 *      y       - the y coordinate of the upper-left outer corner of the frame
 *      w       - the width of the frame window w/o border
 *      h       - the height of the frame window w/o border
 *
 *  Special Considerations:
 *      This routine will check to make sure the window is not completely
 *      off the display, if it is, it'll bring some of it back on.
 *
 *      The fw->frame_XXX variables should NOT be updated with the
 *      values of x,y,w,h prior to calling this routine, since the new
 *      values are compared against the old to see whether a synthetic
 *      ConfigureNotify event should be sent.  (It should be sent if the
 *      window was moved but not resized.)
 *
 ************************************************************************/
void frame_setup_window(
	FvwmWindow *fw, int x, int y, int w, int h,
	Bool do_send_configure_notify)
{
	rectangle g;

	g.x = x;
	g.y = y;
	g.width = w;
	g.height = h;
	__frame_setup_window(fw, &g, do_send_configure_notify, False, False);

	return;
}

void frame_setup_window_app_request(
	FvwmWindow *fw, int x, int y, int w, int h,
	Bool do_send_configure_notify)
{
	rectangle g;

	g.x = x;
	g.y = y;
	g.width = w;
	g.height = h;
	__frame_setup_window(fw, &g, do_send_configure_notify, False, True);

	return;
}

void frame_force_setup_window(
	FvwmWindow *fw, int x, int y, int w, int h,
	Bool do_send_configure_notify)
{
	rectangle g;

	g.x = x;
	g.y = y;
	g.width = w;
	g.height = h;
	__frame_setup_window(fw, &g, do_send_configure_notify, True, False);

	return;
}

/****************************************************************************
 *
 * Sets up the shaped window borders
 *
 ****************************************************************************/
void frame_setup_shape(FvwmWindow *fw, int w, int h, int shape_mode)
{
	rectangle r;
	size_borders b;

	if (!FShapesSupported)
	{
		return;
	}
	if (fw->wShaped != shape_mode)
	{
		fw->wShaped = shape_mode;
		frame_reshape_border(fw);
	}
	if (!shape_mode)
	{
		/* unset shape */
		FShapeCombineMask(
			dpy, FW_W_FRAME(fw), FShapeBounding, 0, 0, None,
			FShapeSet);
	}
	else
	{
		/* shape the window */
		get_window_borders(fw, &b);
		FShapeCombineShape(
			dpy, FW_W_FRAME(fw), FShapeBounding, b.top_left.width,
			b.top_left.height, FW_W(fw), FShapeBounding, FShapeSet);
		if (FW_W_TITLE(fw))
		{
			XRectangle rect;

			/* windows w/ titles */
			r.width = w;
			r.height = h;
			get_title_geometry(fw, &r);
			rect.x = r.x;
			rect.y = r.y;
			rect.width = r.width;
			rect.height = r.height;
			SUPPRESS_UNUSED_VAR_WARNING(rect);
			FShapeCombineRectangles(
				dpy, FW_W_FRAME(fw), FShapeBounding, 0, 0,
				&rect, 1, FShapeUnion, Unsorted);
		}
	}

	return;
}

/* ---------------------------- builtin commands --------------------------- */
