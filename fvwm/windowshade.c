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
#include "libs/gravity.h"
#include "fvwm.h"
#include "externs.h"
#include "misc.h"
#include "cursor.h"
#include "functions.h"
#include "commands.h"
#include "screen.h"
#include "module_interface.h"
#include "geometry.h"
#include "gnome.h"
#include "ewmh.h"
#include "frame.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

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

static char *get_dir_string(int dir)
{
	char *dirs[] = {
		" N",
		" E",
		" S",
		" W",
		"NE",
		"SE",
		"SW",
		"NW"
	};

	if (dir < 0 || dir > 7)
	{
		return "--";
	}

	return dirs[dir];
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
	resize_mode_type resize_mode_x;
	resize_mode_type resize_mode_y;
	rectangle start_g;
	rectangle end_g;

fprintf(stderr,"toggle: %s\n", (action) ? action : "default");
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
		toggle = (!IS_SHADED(tmp_win) ||
			  SHADED_DIR(tmp_win) != shade_dir);
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
fprintf(stderr, "toggle %d (%s) state %d (%s) title dir %s\n", toggle, get_dir_string(shade_dir), IS_SHADED(tmp_win), get_dir_string((IS_SHADED(tmp_win))?SHADED_DIR(tmp_win):-1), get_dir_string(GET_TITLE_DIR(tmp_win)));
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
	start_g = tmp_win->frame_g;
	get_unshaded_geometry(tmp_win, &end_g);
print_g("end1 ", &end_g);
	SET_SHADED(tmp_win, toggle);
	if (toggle == 1)
	{
		SET_SHADED_DIR(tmp_win, shade_dir);
		get_shaded_geometry(tmp_win, &end_g, &end_g);
print_g("end2 ", &end_g);
	}

	/*
	 * do the animation
	 */
	resize_mode_x = (DO_SHRINK_WINDOWSHADE(tmp_win)) ?
		RESIZE_MODE_SHRINK : RESIZE_MODE_SCROLL;
	resize_mode_y = resize_mode_x;
	frame_resize(
		tmp_win, &start_g, &end_g, resize_mode_x, resize_mode_y,
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
