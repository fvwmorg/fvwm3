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
#include "libs/FShape.h"
#include "libs/fvwmrect.h"
#include "fvwm.h"
#include "externs.h"
#include "misc.h"
#include "cursor.h"
#include "functions.h"
#include "commands.h"
#include "screen.h"
#include "module_interface.h"
#include "borders.h"
#include "geometry.h"
#include "gnome.h"
#include "focus.h"
#include "ewmh.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

typedef enum
{
	RESIZE_MODE_RESIZE,
	RESIZE_MODE_SHRINK,
	RESIZE_MODE_SCROLL
} resize_mode_type;

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

static void resize_frame(
	FvwmWindow *tmp_win, rectangle *start_g, rectangle *end_g,
	resize_mode_type mode, int anim_steps)
{
	rectangle client_g;
	int nstep;
	int step = 1;
	int parent_grav = NorthWestGravity;
	int client_grav = NorthWestGravity;
	int title_grav = NorthWestGravity;
	int move_parent_too = False;
	rectangle frame_g;
	rectangle parent_g;
	rectangle shape_g;
	rectangle delta_g;
	rectangle diff;
	rectangle pdiff;
	rectangle sdiff;
	size_borders b;
	FvwmWindow *sf;
	static Window shape_w = None;

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
	if (start_g->width < 1)
	{
		start_g->width = 1;
	}
	if (start_g->height < 1)
	{
		start_g->height = 1;
	}
	if (end_g->width < 1)
	{
		end_g->width = 1;
	}
	if (end_g->height < 1)
	{
		end_g->height = 1;
	}
	if (mode == RESIZE_MODE_RESIZE)
	{
		/* no animated resizing */
		anim_steps = 0;
	}

	/* calcuate the number of steps */
	{
		int wdiff;
		int hdiff;
		int whdiff;

		wdiff = abs(end_g->width - start_g->width);
		hdiff = abs(end_g->height - start_g->height);
		whdiff = max(wdiff, hdiff);
		if (whdiff == 0)
		{
			return;
		}
		if (anim_steps < 0)
		{
			anim_steps = (wdiff - 1) / -anim_steps;
		}
		else if (anim_steps > 0 && anim_steps >= whdiff)
		{
			anim_steps = whdiff - 1;
		}
	}

	/*!!! calculate gravities for the window parts */

	delta_g.x = end_g->x - start_g->x;
	delta_g.y = end_g->y - start_g->y;
	delta_g.width = end_g->width - start_g->width;
	delta_g.height = end_g->height - start_g->height;

	/*!!! calculate order in which to do things */

	if (anim_steps)
	{
		title_grav = get_title_gravity(tmp_win);
		parent_grav = title_grav;
		if (mode == RESIZE_MODE_SCROLL)
		{
			client_grav = (delta_g.x > 0) ?
				NorthWestGravity : SouthEastGravity;
		}
		else
		{
			client_grav = (delta_g.x > 0) ?
				SouthEastGravity : NorthWestGravity;
		}
		set_decor_gravity(
			tmp_win, title_grav, parent_grav, client_grav);
		move_parent_too = HAS_TITLE_DIR(tmp_win, DIR_S);
	}

	/*!!! preparations before animation */



	/*
	 * prepare some convenience variables
	 */
	frame_g = tmp_win->frame_g;
	get_client_geometry(tmp_win, &client_g);
	sf = get_focus_window();





	/*!!! animation */
	for (nstep = 0; nstep < anim_steps; nstep++)
	{
		rectangle current_g;
		rectangle sdelta_g;

		sdelta_g.x = (delta_g.x * (anim_steps + 1)) / (nstep + 1);
		sdelta_g.y = (delta_g.y * (anim_steps + 1)) / (nstep + 1);
		sdelta_g.width =
			(delta_g.width * (anim_steps + 1)) / (nstep + 1);
		sdelta_g.height =
			(delta_g.height * (anim_steps + 1)) / (nstep + 1);
		current_g.x += sdelta_g.x;
		current_g.y += sdelta_g.y;
		current_g.width += sdelta_g.width;
		current_g.height += sdelta_g.height;
	}









	if (anim_steps == 0)
	{
		/*!!!*/
		/*!!! unshade*/
		{
			XMoveResizeWindow(dpy, tmp_win->w, 0, 0, client_g.width, client_g.height);
			if (delta_g.x > 0)
			{
				XMoveResizeWindow(
					dpy, tmp_win->Parent, b.top_left.width,
					b.top_left.height - client_g.height + 1, client_g.width, client_g.height);
				set_decor_gravity(
					tmp_win, SouthEastGravity,
					SouthEastGravity, NorthWestGravity);
			}
			else
			{
				XMoveResizeWindow(
					dpy, tmp_win->Parent, b.top_left.width,
					b.top_left.height, client_g.width, client_g.height);
				set_decor_gravity(
					tmp_win, NorthWestGravity,
					NorthWestGravity, NorthWestGravity);
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
				XMoveWindow(dpy, tmp_win->w, 0, 1 - client_g.height);
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
	/*!!! post animation actions */
	if (IS_SHADED(tmp_win))
	{
		XRaiseWindow(dpy, tmp_win->decor_w);
	}
	else
	{
		XLowerWindow(dpy, tmp_win->decor_w);
	}





	set_decor_gravity(
		tmp_win, NorthWestGravity, NorthWestGravity, NorthWestGravity);
	/* Finally let SetupFrame take care of the window */
	SetupFrame(
		tmp_win, end_g->x, end_g->y, end_g->width, end_g->height, True);
	tmp_win->frame_g = *end_g;
	/* clean up */
	update_absolute_geometry(tmp_win);
	DrawDecorations(
		tmp_win, DRAW_FRAME | DRAW_BUTTONS, (Scr.Hilite == tmp_win),
		True, None, CLEAR_NONE);

	return;
}

/* ---------------------------- interface functions ------------------------- */

/* ---------------------------- builtin commands ---------------------------- */

/***********************************************************************
 *
 *  WindowShade -- shades or unshades a window (veliaa@rpi.edu)
 *
 *  Args: 1 -- shade, 2 -- unshade  No Arg: toggle
 *
 ***********************************************************************/
void CMD_WindowShade(F_CMD_ARGS)
{
	int toggle;
	int shade_dir;
	resize_mode_type resize_mode;
	rectangle start_g;
	rectangle end_g;

	if (DeferExecution(
		    eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
	{
		return;
	}
	if (tmp_win == NULL || IS_ICONIFIED(tmp_win))
	{
		return;
	}

	/*
	 * parse arguments
	 */
	shade_dir = ParseDirectionArgument(action, NULL, -1);
	if (shade_dir != -1)
	{
		toggle = (SHADED_DIR(tmp_win) != shade_dir);
	}
	else
	{
		toggle = ParseToggleArgument(action, NULL, -1, 0);
		if (toggle == -1 &&
		    GetIntegerArguments(action, NULL, &toggle, 1) > 0)
		{
			if (toggle == 1)
			{
				toggle = 1;
			}
			else if (toggle == 2)
			{
				toggle = 0;
			}
			else
			{
				toggle = -1;
			}
		}
		if (toggle == -1)
		{
			toggle = !(IS_SHADED(tmp_win));
		}
		if (!IS_SHADED(tmp_win) && toggle == 1)
		{
			shade_dir = GET_TITLE_DIR(tmp_win);
		}
		else if (IS_SHADED(tmp_win) && toggle == 0)
		{
			shade_dir = SHADED_DIR(tmp_win);
		}
		else
		{
			shade_dir = -1;
		}
	}
	if (!IS_SHADED(tmp_win) && toggle == 0)
	{
		/* nothing to do */
		return;
	}
	if (IS_SHADED(tmp_win) && toggle == 1 &&
	    shade_dir == SHADED_DIR(tmp_win))
	{
		/* nothing to do */
		return;
	}

	/*
	 * calculate start and end geometries
	 */
	get_unshaded_geometry(tmp_win, &start_g);
	if (IS_SHADED(tmp_win))
	{
		get_shaded_geometry(tmp_win, &start_g, &start_g);
	}
	get_unshaded_geometry(tmp_win, &end_g);
	SET_SHADED(tmp_win, toggle);
	if (toggle == 1)
	{
		SET_SHADED_DIR(tmp_win, shade_dir);
		get_shaded_geometry(tmp_win, &end_g, &end_g);
	}

	/*
	 * do the animation
	 */
	resize_mode = (DO_SHRINK_WINDOWSHADE(tmp_win)) ?
		RESIZE_MODE_SHRINK : RESIZE_MODE_SCROLL;
	resize_frame(
		tmp_win, &start_g, &end_g, resize_mode,
		tmp_win->shade_anim_steps);

	/*
	 * update hints and inform modules
	 */
	BroadcastConfig(M_CONFIGURE_WINDOW, tmp_win);
	BroadcastPacket(
		(toggle == 1) ? M_WINDOWSHADE : M_DEWINDOWSHADE, 3, tmp_win->w,
		tmp_win->frame, (unsigned long)tmp_win);
	FlushAllMessageQueues();
	XSync(dpy, 0);
	EWMH_SetWMState(tmp_win, False);
	GNOME_SetHints(tmp_win);
	GNOME_SetWinArea(tmp_win);

	return;
}

/* set the number or size of shade animation steps, N => steps, Np => pixels */
void CMD_WindowShadeAnimate(F_CMD_ARGS)
{
	char *buf;

	if (!action)
	{
		action = "";
	}
	fvwm_msg(
		ERR, "CMD_WindowShadeAnimate",
		"The WindowShadeAnimate command is obsolete. "
		"Please use 'Style * WindowShadeSteps %s' instead.", action);
	buf = safemalloc(strlen(action) + 32);
	sprintf(buf, "* WindowShadeSteps %s", action);
	action = buf;
	CMD_Style(F_PASS_ARGS);
	free(buf);

	return;
}
