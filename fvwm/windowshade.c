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
#include "borders.h"
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
	frame_move_resize_mode resize_mode;
	rectangle start_g;
	rectangle end_g;
	frame_move_resize_args mr_args;

fprintf(stderr,"toggle: %s\n", (action) ? action : "default");
	if (DeferExecution(
		    eventp,&w,&fw,&context, CRS_SELECT,ButtonRelease))
	{
		return;
	}
	if (fw == NULL || IS_ICONIFIED(fw))
	{
		return;
	}

	/* parse arguments */
	shade_dir = ParseDirectionArgument(action, NULL, -1);
	if (shade_dir != -1)
	{
		toggle = (!IS_SHADED(fw) ||
			  SHADED_DIR(fw) != shade_dir);
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
fprintf(stderr, "toggle %d (%s) state %d (%s) title dir %s\n", toggle, get_dir_string(shade_dir), IS_SHADED(fw), get_dir_string((IS_SHADED(fw))?SHADED_DIR(fw):-1), get_dir_string(GET_TITLE_DIR(fw)));
	if (!IS_SHADED(fw) && toggle == 0)
	{
		/* nothing to do */
		return;
	}
	if (IS_SHADED(fw) && toggle == 1 &&
	    shade_dir == SHADED_DIR(fw))
	{
		/* nothing to do */
		return;
	}

	/* draw the animation */
	start_g = fw->frame_g;
	get_unshaded_geometry(fw, &end_g);
print_g("end1 ", &end_g);
	resize_mode = (DO_SHRINK_WINDOWSHADE(fw)) ?
		FRAME_MR_SHRINK : FRAME_MR_SCROLL;
	mr_args = frame_create_move_resize_args(
		fw, resize_mode, &start_g, &end_g, fw->shade_anim_steps);
	frame_move_resize(fw, mr_args);
	frame_free_move_resize_args(mr_args);

	/* Update the window state after the animation.  The animation code
	 * needs to know the old window state to work properly. */
	SET_SHADED(fw, toggle);
	if (toggle == 1)
	{
		SET_SHADED_DIR(fw, shade_dir);
		get_shaded_geometry(fw, &end_g, &end_g);
print_g("end2 ", &end_g);
	}

	/* update hints and inform modules */
	BroadcastConfig(M_CONFIGURE_WINDOW, fw);
	BroadcastPacket(
		(toggle == 1) ? M_WINDOWSHADE : M_DEWINDOWSHADE, 3, FW_W(fw),
		FW_W_FRAME(fw), (unsigned long)fw);
	FlushAllMessageQueues();
	XFlush(dpy);
	EWMH_SetWMState(fw, False);
	GNOME_SetHints(fw);
	GNOME_SetWinArea(fw);

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
