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
#include "gnome.h"
#include "focus.h"
#include "ewmh.h"
#include "borders.h"
#include "frame.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

typedef struct
{
	/* filled when args are created */
	frame_move_resize_mode mode;
	frame_decor_gravities_type grav;
	size_borders b_g;
	rectangle curr_sidebar_g;
	rectangle start_g;
	rectangle end_g;
	rectangle delta_g;
	rectangle current_g;
	int anim_steps;
	Window w_with_focus;
	int current_step;
	int curr_titlebar_compression;
	/* used later */
	int next_titlebar_compression;
	rectangle next_sidebar_g;
	rectangle next_g;
	rectangle dstep_g;
	struct
	{
		/* filled when args are created */
		unsigned was_hidden : 1;
		/* used later */
		unsigned is_hidden : 1;
	} flags;
} mr_args_internal;

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

/* windows used to hide animation steps */
static struct
{
	Window parent;
	Window w[4];
} hide_wins;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

/*!!!remove*/
void print_g(char *text, rectangle *g)
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

static void combine_decor_gravities(
	frame_decor_gravities_type *ret_grav,
	frame_decor_gravities_type *grav_x,
	frame_decor_gravities_type *grav_y)
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
	frame_decor_gravities_type *ret_grav, direction_type title_dir,
	frame_move_resize_mode axis_mode, direction_type neg_dir,
	direction_type pos_dir)
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
	case FRAME_MR_SCROLL:
		ret_grav->client_grav = pos_grav;
		break;
	case FRAME_MR_SHRINK:
	case FRAME_MR_OPAQUE:
	case FRAME_MR_FORCE_SETUP:
	case FRAME_MR_SETUP:
		ret_grav->client_grav = neg_grav;
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

	if (!HAS_BORDER(fw))
	{
		return;
	}
	frame_get_sidebar_geometry(
		fw, NULL, frame_g, &sidebar_g, &dummy, &dummy);
	for (part = PART_BORDER_N; (part & setup_parts & PART_FRAME);
	     part <<= 1)
	{
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

static void frame_setup_title_bar(
	FvwmWindow *fw, rectangle *frame_g, window_parts setup_parts,
	rectangle *diff_g)
{
        frame_title_layout_type title_layout;
        int i;

	if (!HAS_TITLE(fw))
	{
		return;
	}
        frame_get_title_bar_dimensions(fw, frame_g, diff_g, &title_layout);
        /* configure buttons */
	for (i = 0; i < NUMBER_OF_BUTTONS; i++)
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

static void frame_setup_window_internal(
	FvwmWindow *fw, rectangle *frame_g, Bool do_send_configure_notify,
	Bool do_force)
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
	if (new_g.width != fw->frame_g.width ||
	    new_g.height != fw->frame_g.height)
	{
		is_resized = True;
	}
	if (new_g.x != fw->frame_g.x || new_g.y != fw->frame_g.y)
	{
		is_moved = True;
	}
	/* setup the window */
	if (is_resized || do_force)
	{
		mr_args = frame_create_move_resize_args(
			fw, (do_force) ? FRAME_MR_FORCE_SETUP : FRAME_MR_SETUP,
                        NULL, &new_g, 0);
		frame_move_resize(fw, mr_args);
		frame_free_move_resize_args(mr_args);
		fw->frame_g = *frame_g;
	}
	else if (is_moved)
	{
		/* inform the application of the change
		 *
		 * According to the July 27, 1988 ICCCM draft, we should send a
		 * synthetic ConfigureNotify event to the client if the window
		 * was moved but not resized. */
		XMoveWindow(dpy, FW_W_FRAME(fw), frame_g->x, frame_g->y);
		fw->frame_g = *frame_g;
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
	/* inform the modules of the change */
	BroadcastConfig(M_CONFIGURE_WINDOW,fw);

	return;
}

/* ---------------------------- interface functions ------------------------- */

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

void frame_get_title_bar_dimensions(
	FvwmWindow *fw, rectangle *frame_g, rectangle *diff_g,
        frame_title_layout_type *title_layout)
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
                tb_length = frame_g->height - b.total_size.height;
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
	}
	else
	{
		tb_length = frame_g->width - b.total_size.width;
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
	}
        /* configure left buttons */
	for (i = 0; i < NUMBER_OF_BUTTONS; i += 2)
	{
		if (FW_W_BUTTON(fw, i) == None)
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
	for (i = 1 + ((NUMBER_OF_BUTTONS - 1) / 2) * 2; i >= 0; i -= 2)
	{
		if (FW_W_BUTTON(fw, i) == None)
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
	if (!HAS_BORDER(fw))
	{
		return;
	}
	/* get the corner size */
	if (borderstyle == NULL)
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

window_parts frame_get_changed_border_parts(
	rectangle *old_sidebar_g, rectangle *new_sidebar_g)
{
	window_parts changed_parts;

	changed_parts = PART_NONE;
	if (old_sidebar_g->x != new_sidebar_g->x)
	{
		changed_parts |= (PART_FRAME & (~PART_BORDER_W));
	}
	if (old_sidebar_g->y != new_sidebar_g->y)
	{
		changed_parts |= (PART_FRAME & (~PART_BORDER_N));
	}
	if (old_sidebar_g->width != new_sidebar_g->width)
	{
		changed_parts |=
			PART_BORDER_N |
			PART_BORDER_NE |
			PART_BORDER_E |
			PART_BORDER_SE |
			PART_BORDER_S;
	}
	if (old_sidebar_g->height != new_sidebar_g->height)
	{
		changed_parts |=
			PART_BORDER_W |
			PART_BORDER_SW |
			PART_BORDER_S |
			PART_BORDER_SE |
			PART_BORDER_E;
	}

	return changed_parts;
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

		for (i = 0; i < NUMBER_OF_BUTTONS; i++)
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
	rectangle *start_g, rectangle *end_g, int anim_steps)
{
	mr_args_internal *mra;
	Bool dummy;
	int whdiff;

/*!!!disable animated window shading for now*/if (mr_mode==FRAME_MR_SHRINK||mr_mode==FRAME_MR_SCROLL)mr_mode=FRAME_MR_FORCE_SETUP;
	mra = (mr_args_internal *)safecalloc(1, sizeof(mr_args_internal));
	mra->mode = mr_mode;
	get_window_borders(fw, &mra->b_g);
	mra->start_g = (start_g != NULL) ? *start_g : fw->frame_g;
	frame_get_sidebar_geometry(
		fw, NULL, &mra->start_g, &mra->curr_sidebar_g, &dummy, &dummy);
	mra->end_g = *end_g;
	mra->current_g = mra->start_g;
	mra->next_g = mra->end_g;
	mra->w_with_focus = (fw == get_focus_window()) ? FW_W(fw) : None;
	mra->flags.was_hidden =
		(frame_is_parent_hidden(fw, &mra->start_g) == True);
	mra->curr_titlebar_compression =
		frame_get_titlebar_compression(fw, &mra->start_g);
	fvwmrect_subtract_rectangles(
		&mra->delta_g, &mra->end_g, &mra->start_g);
	frame_reparent_hide_windows(FW_W_FRAME(fw));
	/* Set gravities for the window parts. */
	frame_get_resize_decor_gravities(
		&mra->grav, GET_TITLE_DIR(fw), mra->mode);
	frame_set_decor_gravities(fw, &mra->grav);
	/* calcuate the number of animation steps */
	switch (mra->mode)
	{
	case FRAME_MR_SHRINK:
	case FRAME_MR_SCROLL:
		whdiff = max(abs(mra->delta_g.width), abs(mra->delta_g.height));
		if (whdiff == 0)
		{
			mra->anim_steps = 0;
		}
		else if (mra->anim_steps < 0)
		{
			mra->anim_steps = (whdiff - 1) / -mra->anim_steps;
		}
		else if (mra->anim_steps > 0 && mra->anim_steps >= whdiff)
		{
			mra->anim_steps = whdiff - 1;
		}
		break;
	case FRAME_MR_FORCE_SETUP:
	case FRAME_MR_SETUP:
	case FRAME_MR_OPAQUE:
	default:
		mra->anim_steps = 0;
		break;
	}
	mra->anim_steps++;

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

/* Prepares the structure for the next animation step. */
static void frame_next_move_resize_args(
	frame_move_resize_args mr_args)
{
	mr_args_internal *mra;

	mra = (mr_args_internal *)mr_args;
	mra->curr_sidebar_g = mra->next_sidebar_g;
	mra->current_g = mra->next_g;
	mra->flags.was_hidden = mra->flags.is_hidden;
	mra->curr_titlebar_compression = mra->next_titlebar_compression;

	return;
}

/* Destroys the structure allocated with frame_create_move_resize_args().  Does
 * some clean up operations on the modified window first. */
void frame_free_move_resize_args(
	frame_move_resize_args mr_args)
{
	mr_args_internal *mra;
	FvwmWindow *sf;

	mra = (mr_args_internal *)mr_args;
	frame_reparent_hide_windows(Scr.Root);
	if (mra->w_with_focus != None)
	{
		/* domivogt (28-Dec-1999): For some reason the XMoveResize() on
		 * the frame window removes the input focus from the client
		 * window.  I have no idea why, but if we explicitly restore
		 * the focus here everything works fine. */
		FOCUS_SET(mra->w_with_focus);
	}
	/* In case the window geometry now overlaps the focused window. */
	sf = get_focus_window();
	if (sf != NULL)
	{
		focus_grab_buttons(sf, True);
	}
	/* free the memory */
	free(mr_args);

	return;
}

static void frame_hide_changing_window_parts(
	mr_args_internal *mra)
{
	int x_add;
	int y_add;
	int i;

	/* cover top/left borders */
	XMoveResizeWindow(
		dpy, hide_wins.w[0], 0, 0, mra->current_g.width,
		mra->b_g.top_left.height);
	XMoveResizeWindow(
		dpy, hide_wins.w[1], 0, 0, mra->b_g.top_left.width,
		mra->current_g.height);
	/* cover bottom/right borders and possibly part of the client */
	x_add = (mra->dstep_g.width < 0) ? -mra->dstep_g.width : 0;
	y_add = (mra->dstep_g.height < 0) ? -mra->dstep_g.height : 0;
	XMoveResizeWindow(
		dpy, hide_wins.w[2],
		0, mra->current_g.height - mra->b_g.bottom_right.height - y_add,
		mra->current_g.width, mra->b_g.bottom_right.height + y_add);
	XMoveResizeWindow(
		dpy, hide_wins.w[3],
		mra->current_g.width - mra->b_g.bottom_right.width - x_add, 0,
		mra->b_g.bottom_right.width + x_add, mra->current_g.height);
	for (i = 0; i < 4; i++)
	{
		XMapWindow(dpy, hide_wins.w[i]);
	}

	return;
}

static void frame_move_resize_step(
	FvwmWindow *fw, mr_args_internal *mra)
{
	window_parts setup_parts;
	XSetWindowAttributes xswa;
	Bool dummy;
        Bool is_setup;
        Bool is_client_resizing = False;
        Bool do_force_static_gravity = False;
        int do_force;
	int i;
	int w;
	int h;
	struct
	{
		unsigned do_hide_parent : 1;
		unsigned do_unhide_parent : 1;
	} flags;

	/* preparations */
        is_setup = (mra->mode == FRAME_MR_FORCE_SETUP ||
		   mra->mode == FRAME_MR_SETUP) ? True : False;
	if (is_setup == True || mra->mode == FRAME_MR_OPAQUE)
	{
		is_client_resizing = True;
	}
        do_force = (mra->mode == FRAME_MR_FORCE_SETUP);
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
	mra->flags.is_hidden =
		(frame_is_parent_hidden(fw, &mra->next_g) == True);
	flags.do_hide_parent =
		(!mra->flags.was_hidden && mra->flags.is_hidden);
	flags.do_unhide_parent =
		(mra->flags.was_hidden && !mra->flags.is_hidden);
        if (is_setup && mra->dstep_g.x == 0 && mra->dstep_g.y == 0 &&
            mra->dstep_g.width == 0 && mra->dstep_g.height == 0)
        {
                /* The caller has already set the frame geometry.  Use
                 * StaticGravity so the sub windows are not moved to funny
                 * places later. */
                do_force_static_gravity = True;
                do_force = 1;
        }
	/*
	 * resize everything
	 */
	frame_hide_changing_window_parts(mra);
	/* take care of hiding or unhiding the parent */
	if (flags.do_unhide_parent)
	{
		Window w[2];

		/*!!! update the hidden position of the client and parent*/
		w[0] = hide_wins.w[3];
		w[1] = FW_W_PARENT(fw);
		XRestackWindows(dpy, w, 2);
	}
	else if (flags.do_hide_parent)
	{
		/* When the parent gets hidden, unmap it automatically, lower
		 * it while hidden, then remap it.  Necessary to eliminate
		 * flickering. */
		xswa.win_gravity = UnmapGravity;
		XChangeWindowAttributes(
			dpy, FW_W_PARENT(fw), CWWinGravity, &xswa);
	}
        if (do_force_static_gravity == True)
        {
                frame_decor_gravities_type grav;
                grav.decor_grav = StaticGravity;
                grav.title_grav = StaticGravity;
                grav.lbutton_grav = StaticGravity;
                grav.rbutton_grav = StaticGravity;
                grav.parent_grav = StaticGravity;
                grav.client_grav = StaticGravity;
                frame_set_decor_gravities(fw, &grav);
        }
	/* setup the title bar */
	setup_parts = PART_TITLE;
	if (mra->curr_titlebar_compression != mra->next_titlebar_compression ||
            mra->mode == FRAME_MR_FORCE_SETUP)
	{
		setup_parts |= PART_BUTTONS;
	}
	frame_setup_title_bar(fw, &mra->next_g, setup_parts, &mra->dstep_g);
	/* setup the border */
	if (mra->mode == FRAME_MR_SETUP || mra->mode == FRAME_MR_FORCE_SETUP)
	{
		setup_parts = PART_ALL;
	}
	else
	{
		setup_parts = frame_get_changed_border_parts(
			&mra->curr_sidebar_g, &mra->next_sidebar_g);
	}
	frame_setup_border(fw, &mra->next_g, setup_parts, &mra->dstep_g);
	/* draw the border and the titlebar */
	draw_decorations_with_geom(
		fw, PART_ALL, (mra->w_with_focus != None) ? True : False,
		do_force, CLEAR_ALL, &mra->current_g, &mra->next_g);
	/* setup the client and the parent windows */
	w = mra->next_g.width - mra->b_g.total_size.width;
	if (w < 1)
	{
		w = 1;
	}
	h = mra->next_g.height - mra->b_g.total_size.height;
	if (h < 1)
	{
		h = 1;
	}
	/* setup the parent, the frame and the client window */
	if (is_client_resizing)
	{
		XMoveResizeWindow(dpy, FW_W(fw), 0, 0, w, h);
		/* reduces flickering */
		if (is_setup == True)
		{
			usleep(1000);
		}
		XSync(dpy, 0);
	}
	XMoveResizeWindow(
		dpy, FW_W_PARENT(fw), mra->b_g.top_left.width,
		mra->b_g.top_left.height, w, h);
	XMoveResizeWindow(
		dpy, FW_W_FRAME(fw), mra->next_g.x, mra->next_g.y,
		mra->next_g.width, mra->next_g.height);
        /* restore the old gravities */
        if (do_force_static_gravity == True)
        {
                frame_set_decor_gravities(fw, &mra->grav);
        }
	/* finish hiding the parent */
	if (flags.do_hide_parent)
	{
		xswa.win_gravity = mra->grav.parent_grav;
		XChangeWindowAttributes(
			dpy, FW_W_PARENT(fw), CWWinGravity, &xswa);
		/*!!! update the hidden position of the client and parent*/
		XLowerWindow(dpy, FW_W_PARENT(fw));
		XMapWindow(dpy, FW_W_PARENT(fw));
	}
	/* update the window shape*/
        /*!!! may have to change this for window shading */
        if (FShapesSupported && fw->wShaped)
        {
                frame_setup_shape(fw, mra->next_g.width, mra->next_g.height);
        }
	fw->frame_g = mra->next_g;

	return;
}

void frame_move_resize(
	FvwmWindow *fw, frame_move_resize_args mr_args)
{
	mr_args_internal *mra;
	Bool is_grabbed;
	int i;

	mra = (mr_args_internal *)mr_args;
#if 0
	print_g("start", &mra->start_g);
	print_g("end  ", &mra->end_g);
#endif
	/* freeze the cursor shape; otherwise it may flash to a different shape
	 * during the animation */
	is_grabbed = GrabEm(0, GRAB_FREEZE_CURSOR);
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
	/* clean up */
	fw->frame_g = mra->end_g;
	update_absolute_geometry(fw);

	return;
}

/* Calculates the gravities for the various parts of the decor through ret_grav.
 * This can be passed to frame_set_decor_gravities.
 *
 * title_dir
 *   The direction of the title in the frame.
 * rmode
 *   The mode for the resize operation
 */
void frame_get_resize_decor_gravities(
	frame_decor_gravities_type *ret_grav, direction_type title_dir,
	frame_move_resize_mode rmode)
{
	frame_decor_gravities_type grav_x;
	frame_decor_gravities_type grav_y;
	direction_type title_dir_x;
	direction_type title_dir_y;

	gravity_split_xy_dir(&title_dir_x, &title_dir_y, title_dir);
	get_resize_decor_gravities_one_axis(
		&grav_x, title_dir_x, rmode, DIR_W, DIR_E);
	get_resize_decor_gravities_one_axis(
		&grav_y, title_dir_y, rmode, DIR_N, DIR_S);
	combine_decor_gravities(ret_grav, &grav_x, &grav_y);

	return;
}

/* sets the gravity for the various parts of the window */
void frame_set_decor_gravities(
	FvwmWindow *fw, frame_decor_gravities_type *grav)
{
	int valuemask;
	int valuemask2;
	XSetWindowAttributes xcwa;
	int i;

	/* using bit gravity can reduce redrawing dramatically */
	valuemask = CWWinGravity;
	valuemask2 = CWWinGravity | CWBitGravity;
	xcwa.win_gravity = grav->client_grav;
	xcwa.bit_gravity = grav->client_grav;
	XChangeWindowAttributes(dpy, FW_W(fw), valuemask2, &xcwa);
	xcwa.win_gravity = grav->parent_grav;
	xcwa.bit_gravity = grav->parent_grav;
	XChangeWindowAttributes(dpy, FW_W_PARENT(fw), valuemask, &xcwa);
	valuemask = CWWinGravity;
	if (!HAS_TITLE(fw))
	{
		return;
	}
	xcwa.win_gravity = grav->title_grav;
	xcwa.win_gravity = grav->title_grav;
	XChangeWindowAttributes(dpy, FW_W_TITLE(fw), valuemask2, &xcwa);
	xcwa.win_gravity = grav->lbutton_grav;
	for (i = 0; i < NUMBER_OF_BUTTONS; i += 2)
	{
		if (FW_W_BUTTON(fw, i))
		{
			XChangeWindowAttributes(
				dpy, FW_W_BUTTON(fw, i), valuemask, &xcwa);
		}
	}
	xcwa.win_gravity = grav->rbutton_grav;
	for (i = 1; i < NUMBER_OF_BUTTONS; i += 2)
	{
		if (FW_W_BUTTON(fw, i))
		{
			XChangeWindowAttributes(
				dpy, FW_W_BUTTON(fw, i), valuemask, &xcwa);
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
	frame_setup_window_internal(
		fw, &g, do_send_configure_notify, False);

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
	frame_setup_window_internal(
		fw, &g, do_send_configure_notify, True);

	return;
}

/****************************************************************************
 *
 * Sets up the shaped window borders
 *
 ****************************************************************************/
void frame_setup_shape(FvwmWindow *fw, int w, int h)
{
	XRectangle rect;
	rectangle r;
	size_borders b;

	if (!FShapesSupported)
	{
		return;
	}
	if (!fw->wShaped)
	{
		/* unset shape */
		FShapeCombineMask(
			dpy, FW_W_FRAME(fw), FShapeBounding, 0, 0, None,
			FShapeSet);
		return;
	}
	/* shape the window */
	get_window_borders(fw, &b);
	FShapeCombineShape(
		dpy, FW_W_FRAME(fw), FShapeBounding, b.top_left.width,
		b.top_left.height, FW_W(fw), FShapeBounding, FShapeSet);
	if (FW_W_TITLE(fw))
	{
		/* windows w/ titles */
		r.width = w;
		r.height = h;
		get_title_geometry(fw, &r);
		rect.x = r.x;
		rect.y = r.y;
		rect.width = r.width;
		rect.height = r.height;
		FShapeCombineRectangles(
			dpy, FW_W_FRAME(fw), FShapeBounding, 0, 0, &rect, 1,
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
				dpy, FW_W_PARENT(fw), b.top_left.width, b.top_left.height, client_g.width, 1);
			XMoveWindow(
				dpy, FW_W(fw), 0,
				(client_grav == SouthEastGravity) ? -client_g.height + 1 : 0);
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
			XResizeWindow(dpy, FW_W(fw), client_g.width, client_g.height);
			/* make the decor window the full size before the animation unveils it */
			fw->frame_g.width = end_g->width;
			fw->frame_g.height = end_g->height;
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
				if (FShapesSupported && fw->wShaped && shape_w)
				{
					FShapeCombineShape(
						dpy, shape_w, FShapeBounding, shape_g.x, shape_g.y, FW_W(fw),
						FShapeBounding, FShapeSet);
					if (FW_W_TITLE(fw))
					{
						XRectangle rect;

						/* windows w/ titles */
						rect.x = b.top_left.width;
						rect.y = (delta_g.x > 0) ?
							frame_g.height - b.bottom_right.height : 0;
						rect.width = frame_g.width - b.total_size.width;
						rect.height = fw->title_thickness;
						FShapeCombineRectangles(
							dpy, shape_w, FShapeBounding, 0, 0, &rect, 1, FShapeUnion,
							Unsorted);
					}
				}
				if (move_parent_too)
				{
					if (FShapesSupported && fw->wShaped && shape_w)
					{
						FShapeCombineShape(
							dpy, FW_W_FRAME(fw), FShapeBounding, 0, 0, shape_w,
							FShapeBounding, FShapeUnion);
					}
					XMoveResizeWindow(
						dpy, FW_W_PARENT(fw), parent_g.x, parent_g.y, parent_g.width,
						parent_g.height);
				}
				else
				{
					XResizeWindow(dpy, FW_W_PARENT(fw), parent_g.width, parent_g.height);
				}
				XMoveResizeWindow(
					dpy, FW_W_FRAME(fw), frame_g.x, frame_g.y, frame_g.width,
					frame_g.height);
				if (FShapesSupported && fw->wShaped && shape_w)
				{
					FShapeCombineShape(
						dpy, FW_W_FRAME(fw), FShapeBounding, 0, 0, shape_w, FShapeBounding,
						FShapeSet);
				}
				DrawDecorations(
					fw, PART_FRAME, sf == fw, True, None, CLEAR_NONE);
				FlushAllMessageQueues();
				XFlush(dpy);
			}
			if (client_grav == SouthEastGravity && move_parent_too)
			{
				/* We must finish above loop to the very end for this special case.
				 * Otherwise there is a visible jump of the client window. */
				XMoveResizeWindow(
					dpy, FW_W_PARENT(fw), parent_g.x,
					b.top_left.height - (end_g->height - frame_g.height), parent_g.width,
					client_g.height);
				XMoveResizeWindow(
					dpy, FW_W_FRAME(fw), end_g->x, end_g->y, end_g->width, end_g->height);
			}
			else
			{
				XMoveWindow(dpy, FW_W(fw), 0, 0);
			}
			fw->frame_g.height = end_g->height + 1;
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
				if (FShapesSupported && fw->wShaped &&
				    shape_w)
				{
					FShapeCombineShape(
						dpy, shape_w, FShapeBounding,
						shape_g.x, shape_g.y,
						FW_W(fw), FShapeBounding,
						FShapeSet);
					if (FW_W_TITLE(fw))
					{
						XRectangle rect;

						/* windows w/ titles */
						rect.x = b.top_left.width;
						rect.y = (delta_g.x > 0) ?
							frame_g.height -
							b.bottom_right.height :
							0;
						rect.width =
							mra->start_g.width -
							b.total_size.width;
						rect.height =
							fw->title_thickness;
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
					    fw->wShaped && shape_w)
					{
						FShapeCombineShape(
							dpy, FW_W_FRAME(fw),
							FShapeBounding, 0, 0,
							shape_w, FShapeBounding,
							FShapeUnion);
					}
					XMoveResizeWindow(
						dpy, FW_W_FRAME(fw),
						frame_g.x, frame_g.y,
						frame_g.width, frame_g.height);
					XMoveResizeWindow(
						dpy, FW_W_PARENT(fw),
						parent_g.x, parent_g.y,
						parent_g.width,
						parent_g.height);
				}
				else
				{
					XResizeWindow(
						dpy, FW_W_PARENT(fw),
						parent_g.width,
						parent_g.height);
					XMoveResizeWindow(
						dpy, FW_W_FRAME(fw), frame_g.x,
						frame_g.y, frame_g.width,
						frame_g.height);
				}
				if (FShapesSupported && fw->wShaped &&
				    shape_w)
				{
					FShapeCombineShape(
						dpy, FW_W_FRAME(fw),
						FShapeBounding, 0, 0, shape_w,
						FShapeBounding, FShapeSet);
				}
				FlushAllMessageQueues();
				XFlush(dpy);
			}
			/* All this stuff is necessary to prevent flickering */
			set_decor_gravity(
				fw, title_grav, UnmapGravity, client_grav);
			XMoveResizeWindow(
				dpy, FW_W_FRAME(fw), end_g->x, end_g->y,
				end_g->width, end_g->height);
			XResizeWindow(dpy, FW_W_PARENT(fw), client_g.width, 1);
			XMapWindow(dpy, FW_W_PARENT(fw));

			if (delta_g.x > 0)
			{
				XMoveWindow(
					dpy, FW_W(fw), 0,
					1 - client_g.height);
			}
			else
			{
				XMoveWindow(dpy, FW_W(fw), 0, 0);
			}

			/* domivogt (28-Dec-1999): For some reason the
			 * XMoveResize() on the frame window removes the input
			 * focus from the client window.  I have no idea why,
			 * but if we explicitly restore the focus here
			 * everything works fine. */
			if (sf == fw)
			{
				FOCUS_SET(FW_W(fw));
			}
		} /* shade */
	}
#endif

#if 0
	if (mra->anim_steps == 1)
	{
		/*!!!*/
		/*!!! unshade*/
		{
			XMoveResizeWindow(
				dpy, FW_W(fw), 0, 0, client_g.width,
				client_g.height);
			if (delta_g.x > 0)
			{
				XMoveResizeWindow(
					dpy, FW_W_PARENT(fw), b.top_left.width,
					b.top_left.height - client_g.height + 1,
					client_g.width, client_g.height);
#if 0
				frame_set_decor_gravities(
					fw, SouthEastGravity,
					SouthEastGravity, NorthWestGravity);
#endif
			}
			else
			{
				XMoveResizeWindow(
					dpy, FW_W_PARENT(fw), b.top_left.width,
					b.top_left.height, client_g.width,
					client_g.height);
#if 0
				set_decor_gravity(
					fw, NorthWestGravity,
					NorthWestGravity, NorthWestGravity);
#endif
			}
			XMoveResizeWindow(
				dpy, FW_W_FRAME(fw), mra->end_g.x, mra->end_g.y,
				mra->end_g.width, mra->end_g.height);
			fw->frame_g.height = mra->end_g.height + 1;
		}
		/*!!! shade*/
		{
			/* All this stuff is necessary to prevent flickering */
#if 0
			set_decor_gravity(
				fw, title_grav, UnmapGravity, client_grav);
#endif
			XMoveResizeWindow(
				dpy, FW_W_FRAME(fw), mra->end_g.x, mra->end_g.y,
				mra->end_g.width, mra->end_g.height);
			XResizeWindow(dpy, FW_W_PARENT(fw), client_g.width, 1);
			XMapWindow(dpy, FW_W_PARENT(fw));

			if (delta_g.x > 0)
			{
				XMoveWindow(dpy, FW_W(fw), 0,
					    1 - client_g.height);
			}
			else
			{
				XMoveWindow(dpy, FW_W(fw), 0, 0);
			}

			/* domivogt (28-Dec-1999): For some reason the
			 * XMoveResize() on the frame window removes the input
			 * focus from the client window.  I have no idea why,
			 * but if we explicitly restore the focus here
			 * everything works fine.
			 */
			if (sf == fw)
			{
				FOCUS_SET(FW_W(fw));
			}
		}
	}
#endif
#if 0
	/* prepare a shape window if necessary */
	static Window shape_w = None;
	if (FShapesSupported && shape_w == None && fw->wShaped)
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
#endif
