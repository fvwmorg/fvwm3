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
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "misc.h"
#include "commands.h"
#include "screen.h"
#include "module_list.h"
#include "module_interface.h"
#include "geometry.h"
#include "ewmh.h"
#include "borders.h"
#include "frame.h"
#include "focus.h"

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

/* ---------------------------- builtin commands --------------------------- */

/*
 *
 *  WindowShade -- shades or unshades a window (veliaa@rpi.edu)
 *
 *  Args: 1 -- shade, 2 -- unshade  No Arg: toggle
 *
 */
void CMD_WindowShade(F_CMD_ARGS)
{
	direction_t shade_dir;
	int toggle;
	frame_move_resize_mode resize_mode;
	rectangle start_g;
	rectangle end_g;
	frame_move_resize_args mr_args;
	char *token;
	char *naction;
	Bool do_force_shading;
	Bool has_dir;
	FvwmWindow * const fw = exc->w.fw;

	if (IS_ICONIFIED(fw) || IS_EWMH_FULLSCREEN(fw))
	{
		return;
	}
	token = PeekToken(action, &naction);
	if (StrEquals("shadeagain", token))
	{
		do_force_shading = True;
		action = naction;
		token = PeekToken(action, &naction);
	}
	else
	{
		do_force_shading = False;
	}
	/* parse arguments */
	if (StrEquals("Last", token))
	{
		/* last given instead of a direction will make
		 * fvwm to reuse the last used shading direction.
		 * A new, nevershaded window will have
		 * USED_TITLE_DIR_FOR_SHADING set (in add_window.c:
		 * setup_window_structure)
		 */
		action = naction;
		if (!USED_TITLE_DIR_FOR_SHADING(fw))
		{
			shade_dir = SHADED_DIR(fw);
		}
		else
		{
			shade_dir = DIR_NONE;
		}
	}
	else
	{
		/* parse normal direction if last was not given */
		shade_dir = gravity_parse_dir_argument(action, NULL, -1);
	}

	if (shade_dir >= 0 && shade_dir <= DIR_MASK)
	{
		has_dir = True;
		toggle = (!IS_SHADED(fw) || SHADED_DIR(fw) != shade_dir);
	}
	else
	{
		has_dir = False;
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
			toggle = !(IS_SHADED(fw));
		}
		if (!IS_SHADED(fw) && toggle == 1)
		{
			shade_dir = GET_TITLE_DIR(fw);
		}
		else if (IS_SHADED(fw) && toggle == 0)
		{
			shade_dir = SHADED_DIR(fw);
		}
		else
		{
			shade_dir = -1;
		}
	}
	if (!IS_SHADED(fw) && toggle == 0)
	{
		/* nothing to do */
		return;
	}
	if (IS_SHADED(fw) && toggle == 1)
	{
		if (has_dir == False)
		{
			/* nothing to do */
			return;
		}
		else if (do_force_shading == False)
		{
			toggle = 0;
		}
		else if (shade_dir == SHADED_DIR(fw))
		{
			return;
		}
	}

	if (toggle == 1)
	{
		SET_USED_TITLE_DIR_FOR_SHADING(fw, !has_dir);
	}
	/* draw the animation */
	start_g = fw->g.frame;
	get_unshaded_geometry(fw, &end_g);
	if (toggle == 1)
	{
		get_shaded_geometry_with_dir(fw, &end_g, &end_g, shade_dir);
	}
	resize_mode = (DO_SHRINK_WINDOWSHADE(fw)) ?
		FRAME_MR_SHRINK : FRAME_MR_SCROLL;
	mr_args = frame_create_move_resize_args(
		fw, resize_mode, &start_g, &end_g, fw->shade_anim_steps,
		shade_dir);
	frame_move_resize(fw, mr_args);
	/* Set the new shade value before destroying the args but after the
	 * animation. */
	SET_SHADED(fw, toggle);
	if (toggle == 1)
	{
		SET_SHADED_DIR(fw, shade_dir);
	}
	frame_free_move_resize_args(fw, mr_args);
	border_draw_decorations(
		fw, PART_TITLEBAR, (fw == get_focus_window()) ? True : False,
		0, CLEAR_BUTTONS, NULL, NULL);
	/* update hints and inform modules */
	BroadcastConfig(M_CONFIGURE_WINDOW, fw);
	BroadcastPacket(
		(toggle == 1) ? M_WINDOWSHADE : M_DEWINDOWSHADE, 3,
		(long)FW_W(fw), (long)FW_W_FRAME(fw), (unsigned long)fw);
	FlushAllMessageQueues();
	XFlush(dpy);
	EWMH_SetWMState(fw, False);

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
	/* TA:  FIXME!  xasprintf() */
	buf = xmalloc(strlen(action) + 32);
	sprintf(buf, "* WindowShadeSteps %s", action);
	action = buf;
	CMD_Style(F_PASS_ARGS);
	free(buf);

	return;
}
