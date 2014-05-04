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

/*
 *
 * code for moving and resizing windows
 *
 */

#include "config.h"

#include <stdio.h>
#include <math.h>
#include <X11/keysym.h>

#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/Picture.h"
#include "libs/Grab.h"
#include "libs/Parse.h"
#include "libs/Graphics.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "execcontext.h"
#include "commands.h"
#include "misc.h"
#include "screen.h"
#include "menus.h"
#include "menuparameters.h"
#include "module_list.h"
#include "module_interface.h"
#include "focus.h"
#include "borders.h"
#include "frame.h"
#include "geometry.h"
#include "ewmh.h"
#include "virtual.h"
#include "decorations.h"
#include "events.h"
#include "eventhandler.h"
#include "eventmask.h"
#include "colormaps.h"
#include "update.h"
#include "stack.h"
#include "move_resize.h"
#include "functions.h"
#include "style.h"

/* ----- move globals ----- */

#define MOVE_NORMAL  0x00
#define MOVE_PAGE    0x01
#define MOVE_SCREEN  0x02

/* Animated move stuff added by Greg J. Badros, gjb@cs.washington.edu */

float rgpctMovementDefault[32] =
{
	-.01, 0, .01, .03,.08,.18,.3,.45,.60,.75,.85,.90,.94,.97,.99,1.0
	/* must end in 1.0 */
};
int cmsDelayDefault = 10; /* milliseconds */

/* current geometry of size window */
static rectangle sizew_g =
{
	-1,
	-1,
	-1,
	-1
};

static int move_interactive_finish_button_mask =
	((1<<(NUMBER_OF_EXTENDED_MOUSE_BUTTONS))-1) & ~0x2;
static int move_drag_finish_button_mask =
	((1<<(NUMBER_OF_EXTENDED_MOUSE_BUTTONS))-1) & ~0x3;

/* ----- end of move globals ----- */

/* ----- resize globals ----- */

/* DO NOT USE (STATIC) GLOBALS IN THIS MODULE!
 * Since some functions are called from other modules unwanted side effects
 * (i.e. bugs.) would be created */

extern Window PressedW;

static void draw_move_resize_grid(int x, int  y, int  width, int height);

/* ----- end of resize globals ----- */

/*
 *
 *  Procedure:
 *      draw_move_resize_grid - move a window outline
 *
 *  Inputs:
 *      root        - the window we are outlining
 *      x           - upper left x coordinate
 *      y           - upper left y coordinate
 *      width       - the width of the rectangle
 *      height      - the height of the rectangle
 *
 */
static int get_outline_rects(
	XRectangle *rects, int x, int y, int width, int height)
{
	int i;
	int n;
	int m;

	n = 3;
	m = (width - 5) / 2;
	if (m < n)
	{
		n = m;
	}
	m = (height - 5) / 2;
	if (m < n)
	{
		n = m;
	}
	if (n < 1)
	{
		n = 1;
	}

	for (i = 0; i < n; i++)
	{
		rects[i].x = x + i;
		rects[i].y = y + i;
		rects[i].width = width - (i << 1);
		rects[i].height = height - (i << 1);
	}
	if (width - (n << 1) >= 5 && height - (n << 1) >= 5)
	{
		if (width - (n << 1) >= 10)
		{
			int off = (width - (n << 1)) / 3 + n;
			rects[i].x = x + off;
			rects[i].y = y + n;
			rects[i].width = width - (off << 1);
			rects[i].height = height - (n << 1);
			i++;
		}
		if (height - (n << 1) >= 10)
		{
			int off = (height - (n << 1)) / 3 + n;
			rects[i].x = x + n;
			rects[i].y = y + off;
			rects[i].width = width - (n << 1);
			rects[i].height = height - (off << 1);
			i++;
		}
	}

	return i;
}

struct
{
	rectangle geom;
	struct
	{
		unsigned is_enabled : 1;
	} flags;
} move_resize_grid =
{
	{ 0, 0, 0, 0 },
	{ 0 }
};

static void draw_move_resize_grid(int x, int  y, int  width, int height)
{
	int nrects = 0;
	XRectangle rects[10];

	if (move_resize_grid.flags.is_enabled &&
	    x == move_resize_grid.geom.x &&
	    y == move_resize_grid.geom.y &&
	    width == move_resize_grid.geom.width &&
	    height == move_resize_grid.geom.height)
	{
		return;
	}

	memset(rects, 0, 10 * sizeof(XRectangle));
	/* place the resize rectangle into the array of rectangles */
	/* interleave them for best visual look */
	/* draw the new one, if any */
	if (move_resize_grid.flags.is_enabled
	    /*move_resize_grid.geom.width && move_resize_grid.geom.height*/)
	{
		move_resize_grid.flags.is_enabled = 0;
		nrects +=
			get_outline_rects(
				&(rects[0]), move_resize_grid.geom.x,
				move_resize_grid.geom.y,
				move_resize_grid.geom.width,
				move_resize_grid.geom.height);
	}
	if (width && height)
	{
		move_resize_grid.flags.is_enabled = 1;
		move_resize_grid.geom.x = x;
		move_resize_grid.geom.y = y;
		move_resize_grid.geom.width = width;
		move_resize_grid.geom.height = height;
		nrects += get_outline_rects(
			&(rects[nrects]), x, y, width, height);
	}
	if (nrects > 0)
	{
		XDrawRectangles(dpy, Scr.Root, Scr.XorGC, rects, nrects);
		XFlush(dpy);
	}

	return;
}

void switch_move_resize_grid(Bool state)
{
	if (state == False)
	{
		if (move_resize_grid.flags.is_enabled)
		{
			draw_move_resize_grid(0, 0, 0, 0);
		}
		else
		{
			move_resize_grid.geom.x = 0;
			move_resize_grid.geom.y = 0;
			move_resize_grid.geom.width = 0;
			move_resize_grid.geom.height = 0;
		}
	}
	else if (!move_resize_grid.flags.is_enabled)
	{
		if (move_resize_grid.geom.width &&
		    move_resize_grid.geom.height)
		{
			draw_move_resize_grid(
				move_resize_grid.geom.x,
				move_resize_grid.geom.y,
				move_resize_grid.geom.width,
				move_resize_grid.geom.height);
		}
	}

	return;
}

static int ParsePositionArgumentSuffix(
	float *ret_factor, char *suffix, float wfactor, float sfactor)
{
	int n;

	switch (*suffix)
	{
	case 'p':
	case 'P':
		*ret_factor = 1.0;
		n = 1;
		break;
	case 'w':
	case 'W':
		*ret_factor = wfactor;
		n = 1;
		break;
	default:
		*ret_factor = sfactor;
		n = 0;
		break;
	}

	return n;
}

static int __get_shift(int val, float factor)
{
	int shift;

	if (val >= 0)
	{
		shift = (int)(val * factor + 0.5);
	}
	else
	{
		shift = (int)(val * factor - 0.5);
	}

	return shift;
}

/* The vars are named for the x-direction, but this is used for both x and y */
static int GetOnePositionArgument(
	char *s1, int window_pos, int window_size, int *pFinalPos,
	float sfactor, int screen_size, int screen_pos, Bool is_x)
{
	int final_pos;
	float wfactor;

	if (s1 == 0 || *s1 == 0)
	{
		return 0;
	}
	wfactor = (float)window_size / 100;
	/* get start position */
	switch (*s1)
	{
	case 'w':
	case 'W':
		final_pos = window_pos;
		s1++;
		break;
	case 'm':
	case 'M':
	{
	        int x;
		int y;

		if (
			FQueryPointer(
			    dpy, Scr.Root, &JunkRoot, &JunkChild, &JunkX,
			    &JunkY, &x, &y, &JunkMask) == False)
		{
			/* pointer is on a different screen - that's okay here
			 */
			final_pos = 0;
		}
		else
		{
			final_pos = (is_x) ? x : y;
		}
		s1++;
		break;
	}
	default:
		final_pos = screen_pos;
		if (*s1 != 0)
		{
			int val;
			int n;
			float f;

			/* parse value */
			if (sscanf(s1, "-%d%n", &val, &n) >= 1)
			{
				/* i.e. -1, -+1 or --1 */
				final_pos += (screen_size - window_size);
				val = -val;
			}
			else if (
				sscanf(s1, "+%d%n", &val, &n) >= 1 ||
				sscanf(s1, "%d%n", &val, &n) >= 1)
			{
				/* i.e. 1, +1, ++1 or +-1 */
			}
			else
			{
				/* syntax error, ignore rest of string */
				break;
			}
			s1 += n;
			/* parse suffix */
			n = ParsePositionArgumentSuffix(
				&f, s1, wfactor, sfactor);
			s1 += n;
			final_pos += __get_shift(val, f);
		}
		break;
	}
	/* loop over shift arguments */
	while (*s1 != 0)
	{
		int val;
		int n;
		float f;

		/* parse value */
		if (sscanf(s1, "-%d%n", &val, &n) >= 1)
		{
			/* i.e. -1, -+1 or --1 */
			val = -val;
		}
		else if (
			sscanf(s1, "+%d%n", &val, &n) >= 1 ||
			sscanf(s1, "%d%n", &val, &n) >= 1)
		{
			/* i.e. 1, +1, ++1 or +-1 */
		}
		else
		{
			/* syntax error, ignore rest of string */
			break;
		}
		s1 += n;
		/* parse suffix */
		n = ParsePositionArgumentSuffix(&f, s1, wfactor, sfactor);
		s1 += n;
		final_pos += __get_shift(val, f);
	}
	*pFinalPos = final_pos;

	return 1;
}

/* GetMoveArguments is used for Move & AnimatedMove
 * It lets you specify in all the following ways
 *   20  30          Absolute percent position, from left edge and top
 *  -50  50          Absolute percent position, from right edge and top
 *   10p 5p          Absolute pixel position
 *   10p -0p         Absolute pixel position, from bottom
 *  w+5  w-10p       Relative position, right 5%, up ten pixels
 *  m+5  m-10p       Pointer relative position, right 5%, up ten pixels
 * Returns 2 when x & y have parsed without error, 0 otherwise
 */
int GetMoveArguments(
	char **paction, int w, int h, int *pFinalX, int *pFinalY,
	Bool *fWarp, Bool *fPointer, Bool fKeep)
{
	char *s1 = NULL;
	char *s2 = NULL;
	char *token = NULL;
	char *action;
	char *naction;
	int scr_x = 0;
	int scr_y = 0;
	int scr_w = Scr.MyDisplayWidth;
	int scr_h = Scr.MyDisplayHeight;
	Bool use_working_area = True;
	Bool global_flag_parsed = False;
	int retval = 0;

	if (!paction)
	{
		return 0;
	}
	action = *paction;
	action = GetNextToken(action, &s1);
	if (s1 && fPointer && StrEquals(s1, "pointer"))
	{
		*fPointer = True;
		*paction = action;
		free(s1);
		return 0;
	}
	if (s1 && StrEquals(s1, "screen"))
	{
		char *token;
		int scr;
		fscreen_scr_arg arg;
		fscreen_scr_arg* parg;

		free(s1);
		token = PeekToken(action, &action);
		scr = FScreenGetScreenArgument(token, FSCREEN_SPEC_PRIMARY);
		if (scr == FSCREEN_XYPOS)
		{
			arg.xypos.x = *pFinalX;
			arg.xypos.y = *pFinalY;
			parg = &arg;
		}
		else
		{
			parg = NULL;
		}
		FScreenGetScrRect(parg, scr, &scr_x, &scr_y, &scr_w, &scr_h);
		action = GetNextToken(action, &s1);
	}
	action = GetNextToken(action, &s2);
	while (!global_flag_parsed)
	{
		token = PeekToken(action, &naction);
		if (!token)
		{
			global_flag_parsed = True;
			break;
		}

		if (StrEquals(token, "Warp"))
		{
			action = naction;
			if (fWarp)
			{
				*fWarp = True;
			}
		}
		else if (StrEquals(token, "ewmhiwa"))
		{
			use_working_area = False;
			action = naction;
		}
		else
		{
			global_flag_parsed = True;
		}
	}

	if (use_working_area)
	{
		EWMH_GetWorkAreaIntersection(
			NULL, &scr_x, &scr_y, &scr_w, &scr_h,
			EWMH_USE_WORKING_AREA);
	}

	if (s1 != NULL && s2 != NULL)
	{
		retval = 0;
		if (fKeep == True && StrEquals(s1, "keep"))
		{
			retval++;
		}
		else if (
			GetOnePositionArgument(
				s1, *pFinalX, w, pFinalX, (float)scr_w / 100,
				scr_w, scr_x, True))
		{
			retval++;
		}
		if (fKeep == True && StrEquals(s2, "keep"))
		{
			retval++;
		}
		else if (
			GetOnePositionArgument(
				s2, *pFinalY, h, pFinalY, (float)scr_h / 100,
				scr_h, scr_y, False))
		{
			retval++;
		}
		if (retval == 0)
		{
			/* make sure warping is off for interactive moves */
			*fWarp = False;
		}
	}
	else
	{
		/* not enough arguments, switch to current page. */
		while (*pFinalX < 0)
		{
			*pFinalX = Scr.MyDisplayWidth + *pFinalX;
		}
		while (*pFinalY < 0)
		{
			*pFinalY = Scr.MyDisplayHeight + *pFinalY;
		}
	}

	if (s1)
	{
		free(s1);
	}
	if (s2)
	{
		free(s2);
	}
	*paction = action;

	return retval;
}

static int ParseOneResizeArgument(
	char *arg, int scr_size, int wa_size, int dwa_size, int base_size,
	int size_inc, int add_size, int *ret_size)
{
	float factor;
	int val;
	int add_base_size = 0;
	int cch = strlen(arg);
	int tmp_size;

	if (cch == 0)
	{
		return 0;
	}
	if (StrEquals(arg, "keep"))
	{
		/* do not change size */
		return 1;
	}
	if (cch > 1 && arg[cch-2] == 'w' && arg[cch-1] == 'a')
	{
		/* ewmh working area */
		factor = (float)wa_size / 100.0;
		arg[cch-1] = '\0';
	}
	else if (cch > 1 && arg[cch-2] == 'd' && arg[cch-1] == 'a')
	{
		/* ewmh dynamic working area */
		factor = (float)dwa_size / 100.0;
		arg[cch-1] = '\0';
	}
	else if (arg[cch-1] == 'p')
	{
		factor = 1;
		arg[cch-1] = '\0';
	}
	else if (arg[cch-1] == 'c')
	{
		factor = size_inc;
		add_base_size = base_size;
		arg[cch-1] = '\0';
	}
	else
	{
		factor = (float)scr_size / 100.0;
	}
	if (strcmp(arg,"w") == 0)
	{
		/* do not change size */
	}
	else if (sscanf(arg,"w-%d",&val) == 1)
	{
		tmp_size = (int)(val * factor + 0.5);
		if (tmp_size < *ret_size)
		{
			*ret_size -= tmp_size;
		}
		else
		{
			*ret_size = 0;
		}
	}
	else if (sscanf(arg,"w+%d",&val) == 1 || sscanf(arg,"w%d",&val) == 1)
	{
		tmp_size = (int)(val * factor + 0.5);
		if (-tmp_size < *ret_size)
		{
			*ret_size += tmp_size;
		}
		else
		{
			*ret_size = 0;
		}
	}
	else if (sscanf(arg,"-%d",&val) == 1)
	{
		tmp_size = (int)(val * factor + 0.5);
		if (tmp_size < scr_size + add_size)
		{
			*ret_size = scr_size - tmp_size + add_size;
		}
		else
		{
			*ret_size = 0;
		}
	}
	else if (sscanf(arg,"+%d",&val) == 1 || sscanf(arg,"%d",&val) == 1)
	{
		tmp_size = (int)(val * factor + 0.5);
		if (-tmp_size < add_size + add_base_size)
		{
			*ret_size = tmp_size + add_size + add_base_size;
		}
		else
		{
			*ret_size = 0;
		}
	}
	else
	{
		return 0;
	}

	return 1;
}

static int GetResizeArguments(
	char **paction, int x, int y, int w_base, int h_base, int w_inc,
	int h_inc, size_borders *sb, int *pFinalW, int *pFinalH,
	direction_t *ret_dir, Bool *is_direction_fixed,
	Bool *do_warp_to_border, Bool *automatic_border_direction,
	Bool *detect_automatic_direction)
{
	int n;
	char *naction;
	char *token;
	char *tmp_token;
	char *s1;
	char *s2;
	int w_add;
	int h_add;
	int has_frame_option;

	*ret_dir = DIR_NONE;
	*is_direction_fixed = False;
	*do_warp_to_border = False;
	*automatic_border_direction = False;
	*detect_automatic_direction = False;

	if (!paction)
	{
		return 0;
	}
	token = PeekToken(*paction, &naction);
	if (!token)
	{
		return 0;
	}
	if (StrEquals(token, "bottomright") || StrEquals(token, "br"))
	{
		int nx = x + *pFinalW - 1;
		int ny = y + *pFinalH - 1;

		n = GetMoveArguments(
			&naction, 0, 0, &nx, &ny, NULL, NULL, True);
		if (n < 2)
		{
			return 0;
		}
		*pFinalW = nx - x + 1;
		*pFinalH = ny - y + 1;
		*paction = naction;

		return n;
	}
	has_frame_option = 0;
	for ( ; ; token = PeekToken(naction, &naction))
	{
		if (StrEquals(token, "frame"))
		{
			has_frame_option = 1;
		}
		else if (StrEquals(token, "direction"))
		{
			*ret_dir = gravity_parse_dir_argument(
					naction, &naction, DIR_NONE);
			if (*ret_dir != DIR_NONE)
			{
				*is_direction_fixed = True;
			}
			else if (*ret_dir == DIR_NONE)
			{
				tmp_token = PeekToken(naction, &naction);
				if (tmp_token != NULL &&
						StrEquals(tmp_token, "automatic"))
				{
					*detect_automatic_direction = True;
					*is_direction_fixed = True;
				}
			}
		}
		else if (StrEquals(token, "fixeddirection"))
		{
			*is_direction_fixed = True;
		}
		else if (StrEquals(token, "warptoborder"))
		{
			tmp_token = PeekToken(naction, &naction);

			if (tmp_token != NULL &&
				StrEquals(tmp_token, "automatic"))
			{
				*automatic_border_direction = True;
			}
			*do_warp_to_border = True;
		}
		else
		{
			break;
		}
	}
	if (has_frame_option)
	{
		w_add = 0;
		h_add = 0;
	}
	else
	{
		w_add = sb->total_size.width;
		h_add = sb->total_size.height;
	}
	s1 = NULL;
	if (token != NULL)
	{
		s1 = xstrdup(token);
	}
	naction = GetNextToken(naction, &s2);
	if (!s2)
	{
		free(s1);
		return 0;
	}
	*paction = naction;

	n = 0;
	n += ParseOneResizeArgument(
		s1, Scr.MyDisplayWidth,
		Scr.Desktops->ewmh_working_area.width,
		Scr.Desktops->ewmh_dyn_working_area.width, w_base, w_inc,
		w_add, pFinalW);
	n += ParseOneResizeArgument(
		s2, Scr.MyDisplayHeight,
		Scr.Desktops->ewmh_working_area.height,
		Scr.Desktops->ewmh_dyn_working_area.height, h_base, h_inc,
		h_add, pFinalH);

	free(s1);
	free(s2);

	if (n < 2)
	{
		n = 0;
	}

	return n;
}

static int GetResizeMoveArguments(
	char **paction, int w_base, int h_base, int w_inc, int h_inc,
	size_borders *sb, int *pFinalX, int *pFinalY,
	int *pFinalW, int *pFinalH, Bool *fWarp, Bool *fPointer)
{
	char *action = *paction;
	direction_t dir;
	Bool dummy;

	if (!paction)
	{
		return 0;
	}
	if (GetResizeArguments(
		    &action, *pFinalX, *pFinalY, w_base, h_base, w_inc, h_inc,
		    sb, pFinalW, pFinalH, &dir, &dummy, &dummy, &dummy,
		    &dummy) < 2)
	{
		return 0;
	}
	if (GetMoveArguments(
		    &action, *pFinalW, *pFinalH, pFinalX, pFinalY, fWarp,
		    NULL, True) < 2)
	{
		return 0;
	}
	*paction = action;

	return 4;
}

/* Positions the SizeWindow on the current ("moused") xinerama-screen */
static void position_geometry_window(const XEvent *eventp)
{
	int x;
	int y;
	fscreen_scr_arg fscr;

	fscr.mouse_ev = (XEvent *)eventp;
	/* Probably should remove this positioning code from {builtins,fvwm}.c?
	 */
	if (Scr.gs.do_emulate_mwm)
	{
		FScreenCenterOnScreen(
			&fscr, FSCREEN_CURRENT, &x, &y, sizew_g.width,
			sizew_g.height);
	}
	else
	{
		FScreenGetScrRect(&fscr, FSCREEN_CURRENT, &x, &y, NULL, NULL);
	}
	if (x != sizew_g.x || y != sizew_g.y)
	{
		switch_move_resize_grid(False);
		XMoveWindow(dpy, Scr.SizeWindow, x, y);
		switch_move_resize_grid(True);
		sizew_g.x = x;
		sizew_g.y = y;
	}

	return;
}

void resize_geometry_window(void)
{
	int w;
	int h;
	int cset = Scr.DefaultColorset;

	Scr.SizeStringWidth =
		FlocaleTextWidth(Scr.DefaultFont, GEOMETRY_WINDOW_STRING,
				 sizeof(GEOMETRY_WINDOW_STRING) - 1);
	w = Scr.SizeStringWidth + 2 * GEOMETRY_WINDOW_BW;
	h = Scr.DefaultFont->height + 2 * GEOMETRY_WINDOW_BW;
	if (w != sizew_g.width || h != sizew_g.height)
	{
		XResizeWindow(dpy, Scr.SizeWindow, w, h);
		sizew_g.width = w;
		sizew_g.height = h;
	}
	if (cset >= 0)
	{
		SetWindowBackground(
			dpy, Scr.SizeWindow, w, h, &Colorset[cset], Pdepth,
			Scr.StdGC, False);
	}
	else
	{
		XSetWindowBackground(dpy, Scr.SizeWindow, Scr.StdBack);
	}

	return;
}

/*
 *
 *  Procedure:
 *      DisplayPosition - display the position in the dimensions window
 *
 *  Inputs:
 *      tmp_win - the current fvwm window
 *      x, y    - position of the window
 *
 */

static void DisplayPosition(
	const FvwmWindow *tmp_win, const XEvent *eventp, int x, int y,int Init)
{
	char str[100];
	int offset;
	fscreen_scr_arg fscr;
	FlocaleWinString fstr;

	if (Scr.gs.do_hide_position_window)
	{
		return;
	}
	position_geometry_window(eventp);
	/* Translate x,y into local screen coordinates,
	 * in case Xinerama is used. */
	fscr.xypos.x = x;
	fscr.xypos.y = y;
	FScreenTranslateCoordinates(
		NULL, FSCREEN_GLOBAL, &fscr, FSCREEN_XYPOS, &x, &y);
	(void)sprintf(str, GEOMETRY_WINDOW_POS_STRING, x, y);
	if (Init)
	{
		XClearWindow(dpy, Scr.SizeWindow);
	}
	else
	{
		/* just clear indside the relief lines to reduce flicker */
		XClearArea(dpy, Scr.SizeWindow,
			   GEOMETRY_WINDOW_BW, GEOMETRY_WINDOW_BW,
			   Scr.SizeStringWidth, Scr.DefaultFont->height, False);
	}

	if (Pdepth >= 2)
	{
		RelieveRectangle(
			dpy, Scr.SizeWindow, 0, 0,
			Scr.SizeStringWidth + GEOMETRY_WINDOW_BW * 2 - 1,
			Scr.DefaultFont->height + GEOMETRY_WINDOW_BW * 2 - 1,
			Scr.StdReliefGC, Scr.StdShadowGC, GEOMETRY_WINDOW_BW);
	}
	offset = (Scr.SizeStringWidth -
		  FlocaleTextWidth(Scr.DefaultFont, str, strlen(str))) / 2;
	offset += GEOMETRY_WINDOW_BW;

	memset(&fstr, 0, sizeof(fstr));
	if (Scr.DefaultColorset >= 0)
	{
		fstr.colorset = &Colorset[Scr.DefaultColorset];
		fstr.flags.has_colorset = True;
	}
	fstr.str = str;
	fstr.win = Scr.SizeWindow;
	fstr.gc = Scr.StdGC;
	fstr.x = offset;
	fstr.y = Scr.DefaultFont->ascent + GEOMETRY_WINDOW_BW;
	FlocaleDrawString(dpy, Scr.DefaultFont, &fstr, 0);

	return;
}


/*
 *
 *  Procedure:
 *      DisplaySize - display the size in the dimensions window
 *
 *  Inputs:
 *      tmp_win - the current fvwm window
 *      width   - the width of the rubber band
 *      height  - the height of the rubber band
 *
 */
static void DisplaySize(
	const FvwmWindow *tmp_win, const XEvent *eventp, int width,
	int height, Bool Init, Bool resetLast)
{
	char str[100];
	int dwidth,dheight,offset;
	size_borders b;
	static int last_width = 0;
	static int last_height = 0;
	FlocaleWinString fstr;

	if (Scr.gs.do_hide_resize_window)
	{
		return;
	}
	position_geometry_window(eventp);
	if (resetLast)
	{
		last_width = 0;
		last_height = 0;
	}
	if (last_width == width && last_height == height)
	{
		return;
	}
	last_width = width;
	last_height = height;

	get_window_borders(tmp_win, &b);
	dheight = height - b.total_size.height;
	dwidth = width - b.total_size.width;
	dwidth -= tmp_win->hints.base_width;
	dheight -= tmp_win->hints.base_height;
	dwidth /= tmp_win->hints.width_inc;
	dheight /= tmp_win->hints.height_inc;

	(void)sprintf(str, GEOMETRY_WINDOW_SIZE_STRING, dwidth, dheight);
	if (Init)
	{
		XClearWindow(dpy,Scr.SizeWindow);
	}
	else
	{
		/* just clear indside the relief lines to reduce flicker */
		XClearArea(
			dpy, Scr.SizeWindow, GEOMETRY_WINDOW_BW,
			GEOMETRY_WINDOW_BW, Scr.SizeStringWidth,
			Scr.DefaultFont->height, False);
	}

	if (Pdepth >= 2)
	{
		RelieveRectangle(
			dpy, Scr.SizeWindow, 0, 0,
			Scr.SizeStringWidth + GEOMETRY_WINDOW_BW * 2 - 1,
			Scr.DefaultFont->height + GEOMETRY_WINDOW_BW*2 - 1,
			Scr.StdReliefGC, Scr.StdShadowGC, GEOMETRY_WINDOW_BW);
	}
	offset = (Scr.SizeStringWidth -
		  FlocaleTextWidth(Scr.DefaultFont, str, strlen(str))) / 2;
	offset += GEOMETRY_WINDOW_BW;
	memset(&fstr, 0, sizeof(fstr));
	if (Scr.DefaultColorset >= 0)
	{
		fstr.colorset = &Colorset[Scr.DefaultColorset];
		fstr.flags.has_colorset = True;
	}
	fstr.str = str;
	fstr.win = Scr.SizeWindow;
	fstr.gc = Scr.StdGC;
	fstr.x = offset;
	fstr.y = Scr.DefaultFont->ascent + GEOMETRY_WINDOW_BW;
	FlocaleDrawString(dpy, Scr.DefaultFont, &fstr, 0);

	return;
}

static Bool resize_move_window(F_CMD_ARGS)
{
	int FinalX = 0;
	int FinalY = 0;
	int FinalW = 0;
	int FinalH = 0;
	int n;
	int x,y;
	Bool fWarp = False;
	Bool fPointer = False;
	int dx;
	int dy;
	size_borders b;
	FvwmWindow *fw = exc->w.fw;
	Window w = exc->w.w;

	if (!is_function_allowed(F_MOVE, NULL, fw, RQORIG_PROGRAM_US, False))
	{
		return False;
	}
	if (!is_function_allowed(F_RESIZE, NULL, fw, RQORIG_PROGRAM_US, True))
	{
		return False;
	}

	/* gotta have a window */
	w = FW_W_FRAME(fw);
	if (!XGetGeometry(
		    dpy, w, &JunkRoot, &x, &y, (unsigned int*)&FinalW,
		    (unsigned int*)&FinalH, (unsigned int*)&JunkBW,
		    (unsigned int*)&JunkDepth))
	{
		XBell(dpy, 0);
		return False;
	}

	FinalX = x;
	FinalY = y;

	get_window_borders(fw, &b);
	n = GetResizeMoveArguments(
		&action,
		fw->hints.base_width, fw->hints.base_height,
		fw->hints.width_inc, fw->hints.height_inc,
		&b, &FinalX, &FinalY, &FinalW, &FinalH, &fWarp, &fPointer);
	if (n < 4)
	{
		return False;
	}

	if (IS_MAXIMIZED(fw))
	{
		/* must redraw the buttons now so that the 'maximize' button
		 * does not stay depressed. */
		SET_MAXIMIZED(fw, 0);
		border_draw_decorations(
			fw, PART_BUTTONS, (fw == Scr.Hilite), True, CLEAR_ALL,
			NULL, NULL);
	}
	dx = FinalX - fw->g.frame.x;
	dy = FinalY - fw->g.frame.y;
	/* size will be less or equal to requested */
	constrain_size(fw, NULL, &FinalW, &FinalH, 0, 0, 0);
	if (IS_SHADED(fw))
	{
		frame_setup_window(
			fw, FinalX, FinalY, FinalW, fw->g.frame.height, False);
	}
	else
	{
		frame_setup_window(fw, FinalX, FinalY, FinalW, FinalH, True);
	}
	if (fWarp)
	{
		FWarpPointer(
			dpy, None, None, 0, 0, 0, 0, FinalX - x, FinalY - y);
	}
	if (IS_MAXIMIZED(fw))
	{
		fw->g.max.x += dx;
		fw->g.max.y += dy;
	}
	else
	{
		fw->g.normal.x += dx;
		fw->g.normal.y += dy;
	}
	update_absolute_geometry(fw);
	maximize_adjust_offset(fw);
	XFlush(dpy);

	return True;
}

void CMD_ResizeMove(F_CMD_ARGS)
{
	FvwmWindow *fw = exc->w.fw;

	if (IS_EWMH_FULLSCREEN(fw))
	{
		/* do not unmaximize ! */
		CMD_ResizeMoveMaximize(F_PASS_ARGS);
		return;
	}
	resize_move_window(F_PASS_ARGS);

	return;
}

static void InteractiveMove(
	Window *win, const exec_context_t *exc, int *FinalX, int *FinalY,
	Bool do_start_at_pointer)
{
	int origDragX,origDragY,DragX, DragY, DragWidth, DragHeight;
	int XOffset, YOffset;
	Window w;
	Bool do_move_opaque = False;

	w = *win;

	if (Scr.bo.do_install_root_cmap)
	{
		InstallRootColormap();
	}
	else
	{
		InstallFvwmColormap();
	}
	/* warp the pointer to the cursor position from before menu appeared */
	/* domivogt (17-May-1999): an XFlush should not hurt anyway, so do it
	 * unconditionally to remove the external */
	XFlush(dpy);

	if (do_start_at_pointer)
	{
		if (FQueryPointer(
			    dpy, Scr.Root, &JunkRoot, &JunkChild, &DragX,
			    &DragY, &JunkX, &JunkY, &JunkMask) == False)
		{
			/* pointer is on a different screen */
			DragX = 0;
			DragY = 0;
		}
	}
	else
	{
		/* Although a move is usually done with a button depressed we
		 * have to check for ButtonRelease too since the event may be
		 * faked. */
		fev_get_evpos_or_query(
			dpy, Scr.Root, exc->x.elast, &DragX, &DragY);
	}

	MyXGrabServer(dpy);
	if (!XGetGeometry(
		    dpy, w, &JunkRoot, &origDragX, &origDragY,
		    (unsigned int*)&DragWidth, (unsigned int*)&DragHeight,
		    (unsigned int*)&JunkBW, (unsigned int*)&JunkDepth))
	{
		MyXUngrabServer(dpy);
		return;
	}
	MyXGrabKeyboard(dpy);
	if (do_start_at_pointer)
	{
		origDragX = DragX;
		origDragY = DragY;
	}

	if (IS_ICONIFIED(exc->w.fw))
	{
		do_move_opaque = True;
	}
	else if (IS_MAPPED(exc->w.fw))
	{
		float areapct;

		areapct = 100.0;
		areapct *= ((float)DragWidth / (float)Scr.MyDisplayWidth);
		areapct *= ((float)DragHeight / (float)Scr.MyDisplayHeight);
		/* round up */
		areapct += 0.1;
		if (Scr.OpaqueSize < 0 ||
		    (float)areapct <= (float)Scr.OpaqueSize)
		{
			do_move_opaque = True;
		}
	}
	if (do_move_opaque)
	{
		MyXUngrabServer(dpy);
	}
	else
	{
		Scr.flags.is_wire_frame_displayed = True;
	}

	if (!do_move_opaque && IS_ICONIFIED(exc->w.fw))
	{
		XUnmapWindow(dpy,w);
	}

	XOffset = origDragX - DragX;
	YOffset = origDragY - DragY;
	if (!Scr.gs.do_hide_position_window)
	{
		position_geometry_window(NULL);
		XMapRaised(dpy,Scr.SizeWindow);
	}
	__move_loop(
		exc, XOffset, YOffset, DragWidth, DragHeight, FinalX, FinalY,
		do_move_opaque, CRS_MOVE);
	if (!Scr.gs.do_hide_position_window)
	{
		XUnmapWindow(dpy,Scr.SizeWindow);
	}
	if (Scr.bo.do_install_root_cmap)
	{
		UninstallRootColormap();
	}
	else
	{
		UninstallFvwmColormap();
	}

	if (!do_move_opaque)
	{
		int event_types[2] = { EnterNotify, LeaveNotify };

		/* Throw away some events that dont interest us right now. */
		discard_typed_events(2, event_types);
		Scr.flags.is_wire_frame_displayed = False;
		MyXUngrabServer(dpy);
	}
	MyXUngrabKeyboard(dpy);

	return;
}

/* Perform the movement of the window. ppctMovement *must* have a 1.0 entry
 * somewhere in ins list of floats, and movement will stop when it hits a 1.0
 * entry */
static void AnimatedMoveAnyWindow(
	FvwmWindow *fw, Window w, int startX, int startY, int endX,
	int endY, Bool fWarpPointerToo, int cmsDelay, float *ppctMovement,
	MenuRepaintTransparentParameters *pmrtp)
{
	int pointerX, pointerY;
	int currentX, currentY;
	int lastX, lastY;
	int deltaX, deltaY;
	Bool first = True;
	XEvent evdummy;
	unsigned int draw_parts = PART_NONE;

	if (!is_function_allowed(F_MOVE, NULL, fw, RQORIG_PROGRAM_US, False))
	{
		return;
	}

	/* set our defaults */
	if (ppctMovement == NULL)
	{
		ppctMovement = rgpctMovementDefault;
	}
	if (cmsDelay < 0)
	{
		cmsDelay = cmsDelayDefault;
	}

	if (startX < 0 || startY < 0)
	{
		if (
			!XGetGeometry(
				dpy, w, &JunkRoot, &currentX, &currentY,
				(unsigned int*)&JunkWidth,
				(unsigned int*)&JunkHeight,
				(unsigned int*)&JunkBW,
				(unsigned int*)&JunkDepth))
		{
			XBell(dpy, 0);
			return;
		}
		if (startX < 0)
		{
			startX = currentX;
		}
		if (startY < 0)
		{
			startY = currentY;
		}
	}

	deltaX = endX - startX;
	deltaY = endY - startY;
	lastX = startX;
	lastY = startY;

	if (deltaX == 0 && deltaY == 0)
	{
		/* go nowhere fast */
		return;
	}

	if (fw && w == FW_W_FRAME(fw))
	{
		draw_parts = border_get_transparent_decorations_part(fw);
	}

	/* Needed for aborting */
	MyXGrabKeyboard(dpy);
	do
	{
		currentX = startX + deltaX * (*ppctMovement);
		currentY = startY + deltaY * (*ppctMovement);
		if (lastX == currentX && lastY == currentY)
		{
			/* don't waste time in the same spot */
			continue;
		}
		if (pmrtp != NULL)
		{
			update_transparent_menu_bg(
				pmrtp, lastX, lastY, currentX, currentY,
				endX, endY);
		}
		XMoveWindow(dpy,w,currentX,currentY);
		if (pmrtp != NULL)
		{
			repaint_transparent_menu(
				pmrtp, first,
				currentX, currentY, endX, endY, True);
		}
		else if (draw_parts != PART_NONE)
		{
			border_draw_decorations(
				fw, draw_parts,
				((fw == get_focus_window())) ?
				True : False,
				True, CLEAR_ALL, NULL, NULL);
		}
		if (fw && pmrtp == NULL && IS_TEAR_OFF_MENU(fw))
		{
			menu_redraw_transparent_tear_off_menu(fw, False);
		}
		if (fWarpPointerToo == True)
		{
			if (FQueryPointer(
				    dpy, Scr.Root, &JunkRoot, &JunkChild,
				    &JunkX, &JunkY, &pointerX, &pointerY,
				    &JunkMask) == False)
			{
				/* pointer is on a different screen */
				pointerX = currentX;
				pointerY = currentY;
			}
			else
			{
				pointerX += currentX - lastX;
				pointerY += currentY - lastY;
			}
			FWarpPointer(
				dpy, None, Scr.Root, 0, 0, 0, 0, pointerX,
				pointerY);
		}
		if (fw && !IS_SHADED(fw) && !Scr.bo.do_disable_configure_notify)
		{
			/* send configure notify event for windows that care
			 * about their location */
			SendConfigureNotify(
				fw, currentX, currentY,
				fw->g.frame.width,
				fw->g.frame.height, 0, False);
#ifdef FVWM_DEBUG_MSGS
			fvwm_msg(DBG,"AnimatedMoveAnyWindow",
				 "Sent ConfigureNotify (w == %d, h == %d)",
				 fw->g.frame.width,
				 fw->g.frame.height);
#endif
		}
		XFlush(dpy);
		if (fw)
		{
			fw->g.frame.x = currentX;
			fw->g.frame.y = currentY;
			update_absolute_geometry(fw);
			maximize_adjust_offset(fw);
			BroadcastConfig(M_CONFIGURE_WINDOW, fw);
			FlushAllMessageQueues();
		}

		usleep(cmsDelay * 1000); /* usleep takes microseconds */
		/* this didn't work for me -- maybe no longer necessary since
		 * we warn the user when they use > .5 seconds as a
		 * between-frame delay time.
		 *
		 * domivogt (28-apr-1999): That is because the keyboard was not
		 * grabbed. works nicely now.
		 */
		if (FCheckMaskEvent(
			    dpy, ButtonPressMask|ButtonReleaseMask|KeyPressMask,
			    &evdummy))
		{
			/* finish the move immediately */
			if (pmrtp != NULL)
			{
				update_transparent_menu_bg(
					pmrtp, lastX, lastY,
					currentX, currentY, endX, endY);
			}
			XMoveWindow(dpy,w,endX,endY);
			if (pmrtp != NULL)
			{
				repaint_transparent_menu(
					pmrtp, first,
					endX, endY, endX, endY, True);
			}
			break;
		}
		lastX = currentX;
		lastY = currentY;
		first = False;
	} while (*ppctMovement != 1.0 && ppctMovement++);
	MyXUngrabKeyboard(dpy);
	XFlush(dpy);

	return;
}

/* used for moving menus, not a client window */
void AnimatedMoveOfWindow(
	Window w, int startX, int startY, int endX, int endY,
	Bool fWarpPointerToo, int cmsDelay, float *ppctMovement,
	MenuRepaintTransparentParameters *pmrtp)
{
	AnimatedMoveAnyWindow(
		NULL, w, startX, startY, endX, endY, fWarpPointerToo,
		cmsDelay, ppctMovement, pmrtp);

	return;
}

/* used for moving client windows */
void AnimatedMoveFvwmWindow(
	FvwmWindow *fw, Window w, int startX, int startY, int endX,
	int endY, Bool fWarpPointerToo, int cmsDelay, float *ppctMovement)
{
	AnimatedMoveAnyWindow(
		fw, w, startX, startY, endX, endY, fWarpPointerToo,
		cmsDelay, ppctMovement, NULL);

	return;
}

int placement_binding(int button, KeySym keysym, int modifier, char *action)
{
	if (keysym != 0)
	{
		/* fixme */
		fvwm_msg(
			ERR, "placement_binding",
			"sorry, placement keybindings not allowed. yet.");
		return 1;
	}
	if (modifier != 0)
	{
		/* fixme */
		fvwm_msg(
			ERR, "placement_binding",
			"sorry, placement binding modifiers not allowed. yet.");
		return 1;
	}
	if (strcmp(action,"-") == 0 ||
		strcasecmp(action,"CancelPlacement") == 0)
	{
		if (keysym == 0) /* must be button binding */
		{
			if (button == 0)
			{
				move_drag_finish_button_mask = 0;
				move_interactive_finish_button_mask = 0;
			}
			else if (button > 0 && button <=
				 NUMBER_OF_EXTENDED_MOUSE_BUTTONS)
			{
				move_drag_finish_button_mask &=
					~(1<<(button-1));
				move_interactive_finish_button_mask &=
					~(1<<(button-1));
			}
		}
	}
	else if (strcasecmp(action,"CancelPlacementDrag") == 0)
	{
		if (keysym == 0) /* must be button binding */
		{
			if (button == 0)
			{
				move_drag_finish_button_mask = 0;
			}
			else if (button > 0 && button <=
				 NUMBER_OF_EXTENDED_MOUSE_BUTTONS)
			{
				move_drag_finish_button_mask &=
					~(1<<(button-1));
			}
		}
	}
	else if (strcasecmp(action,"CancelPlacementInteractive") == 0)
	{
		if (keysym == 0) /* must be button binding */
		{
			if (button == 0)
			{
				move_interactive_finish_button_mask = 0;
			}
			else if (button > 0 && button <=
				 NUMBER_OF_EXTENDED_MOUSE_BUTTONS)
			{
				move_interactive_finish_button_mask &=
					~(1<<(button-1));
			}
		}
	}
	else if (strcasecmp(action,"PlaceWindow") == 0)
	{
		if (keysym == 0) /* must be button binding */
		{
			if (button == 0)
			{
				move_interactive_finish_button_mask =
				move_drag_finish_button_mask =
					(1<<NUMBER_OF_EXTENDED_MOUSE_BUTTONS)-1;
			}
			else if (button > 0 && button <=
				 NUMBER_OF_EXTENDED_MOUSE_BUTTONS)
			{
				move_drag_finish_button_mask |= (1<<(button-1));
				move_interactive_finish_button_mask |=
					(1<<(button-1));
			}
		}
	}
	else if (strcasecmp(action,"PlaceWindowDrag") == 0)
	{
		if (keysym == 0) /* must be button binding */
		{
			if (button == 0)
			{
				move_drag_finish_button_mask =
					(1<<NUMBER_OF_EXTENDED_MOUSE_BUTTONS)-1;
			}
			else if (button > 0 && button <=
				 NUMBER_OF_EXTENDED_MOUSE_BUTTONS)
			{
				move_drag_finish_button_mask |= (1<<(button-1));
			}
		}
	}
	else if (strcasecmp(action,"PlaceWindowInteractive") == 0)
	{
		if (keysym == 0) /* must be button binding */
		{
			if (button == 0)
			{
				move_interactive_finish_button_mask =
					(1<<NUMBER_OF_EXTENDED_MOUSE_BUTTONS)-1;
			}
			else if (button > 0 && button <=
				 NUMBER_OF_EXTENDED_MOUSE_BUTTONS)
			{
				move_interactive_finish_button_mask |=
					(1<<(button-1));
			}
		}
	}
	else
	{
		fvwm_msg(
			ERR, "placement_binding",
			"invalid action %s", action);
	}

	return 0;
}


/*
 *
 * Start a window move operation
 *
 */
void __move_icon(
	FvwmWindow *fw, int x, int y, int old_x, int old_y,
	Bool do_move_animated, Bool do_warp_pointer)
{
	rectangle gt;
	rectangle gp;
	Bool has_icon_title;
	Bool has_icon_picture;
	Window tw;
	int tx;
	int ty;

	set_icon_position(fw, x, y);
	broadcast_icon_geometry(fw, False);
	has_icon_title = get_visible_icon_title_geometry(fw, &gt);
	has_icon_picture = get_visible_icon_picture_geometry(fw, &gp);
	if (has_icon_picture)
	{
		tw = FW_W_ICON_PIXMAP(fw);
		tx = gp.x;
		ty = gp.y;
	}
	else if (has_icon_title)
	{
		tw = FW_W_ICON_TITLE(fw);
		tx = gt.x;
		ty = gt.y;
	}
	else
	{
		return;
	}
	if (do_move_animated)
	{
		AnimatedMoveOfWindow(
			tw, -1, -1, tx, ty, do_warp_pointer, -1, NULL, NULL);
		do_warp_pointer = 0;
	}
	if (has_icon_title)
	{
		XMoveWindow(dpy, FW_W_ICON_TITLE(fw), gt.x, gt.y);
	}
	if (has_icon_picture)
	{
		XMoveWindow(dpy, FW_W_ICON_PIXMAP(fw), gp.x, gp.y);
		if (fw->Desk == Scr.CurrentDesk)
		{
			XMapWindow(dpy, FW_W_ICON_PIXMAP(fw));
			if (has_icon_title)
			{
				XMapWindow(dpy, FW_W_ICON_TITLE(fw));
			}
		}
	}
	if (do_warp_pointer)
	{
		FWarpPointer(dpy, None, None, 0, 0, 0, 0, x - old_x, y - old_y);
	}

	return;
}

static void __move_window(F_CMD_ARGS, Bool do_animate, int mode)
{
	int FinalX = 0;
	int FinalY = 0;
	int n;
	int x;
	int y;
	int width, height;
	int page_x, page_y;
	Bool fWarp = False;
	Bool fPointer = False;
	int dx;
	int dy;
	FvwmWindow *fw = exc->w.fw;
	Window w;

	if (!is_function_allowed(F_MOVE, NULL, fw, RQORIG_PROGRAM_US, False))
	{
		return;
	}
	/* gotta have a window */
	w = FW_W_FRAME(fw);
	if (IS_ICONIFIED(fw))
	{
		if (FW_W_ICON_PIXMAP(fw) != None)
		{
			w = FW_W_ICON_PIXMAP(fw);
			XUnmapWindow(dpy,FW_W_ICON_TITLE(fw));
		}
		else
		{
			w = FW_W_ICON_TITLE(fw);
		}
		if (w == None && (mode == MOVE_PAGE || mode == MOVE_SCREEN))
		{
			w = FW_W_FRAME(fw);
		}
	}
	if (
		!XGetGeometry(
			dpy, w, &JunkRoot, &x, &y, (unsigned int*)&width,
			(unsigned int*)&height, (unsigned int*)&JunkBW,
			(unsigned int*)&JunkDepth))
	{
		return;
	}
	if (mode == MOVE_PAGE && IS_STICKY_ACROSS_PAGES(fw))
	{
		return;
	}
	if (mode == MOVE_PAGE)
	{
		rectangle r;
		rectangle s;
		rectangle t;

		do_animate = False;
		r.x = x;
		r.y = y;
		r.width = width;
		r.height = height;
		get_absolute_geometry(&t, &r);
		get_page_offset_rectangle(&page_x, &page_y, &t);
		if (!get_page_arguments(action, &page_x, &page_y))
		{
			page_x = Scr.Vx;
			page_y = Scr.Vy;
		}
		s.x = page_x - Scr.Vx;
		s.y = page_y - Scr.Vy;
		s.width = Scr.MyDisplayWidth;
		s.height = Scr.MyDisplayHeight;
		fvwmrect_move_into_rectangle(&r, &s);
		FinalX = r.x;
		FinalY = r.y;
	}
	else if (mode == MOVE_SCREEN)
	{
		rectangle r;
		rectangle s;
		rectangle p;
		int fscreen;

		do_animate = False;
		fscreen = FScreenGetScreenArgument(
			action, FSCREEN_SPEC_CURRENT);
		FScreenGetScrRect(
			NULL, fscreen, &s.x, &s.y, &s.width, &s.height);
		page_x = Scr.Vx;
		page_y = Scr.Vy;
		r.x = x;
		r.y = y;
		r.width = width;
		r.height = height;
		p.x = page_x - Scr.Vx;
		p.y = page_y - Scr.Vy;
		p.width = Scr.MyDisplayWidth;
		p.height = Scr.MyDisplayHeight;
		/* move to page first */
		fvwmrect_move_into_rectangle(&r, &p);
		/* then move to screen */
		fvwmrect_move_into_rectangle(&r, &s);
		FinalX = r.x;
		FinalY = r.y;
	}
	else
	{
		FinalX = x;
		FinalY = y;
		n = GetMoveArguments(
			&action, width, height, &FinalX, &FinalY, &fWarp,
			&fPointer, True);

		if (n != 2 || fPointer)
		{
			InteractiveMove(&w, exc, &FinalX, &FinalY, fPointer);
		}
		else if (IS_ICONIFIED(fw))
		{
			SET_ICON_MOVED(fw, 1);
		}
	}

	if (w == FW_W_FRAME(fw))
	{
		dx = FinalX - fw->g.frame.x;
		dy = FinalY - fw->g.frame.y;
		if (do_animate)
		{
			AnimatedMoveFvwmWindow(
				fw, w, -1, -1, FinalX, FinalY, fWarp, -1,
				NULL);
		}
		frame_setup_window(
			fw, FinalX, FinalY, fw->g.frame.width,
			fw->g.frame.height, True);
		if (fWarp & !do_animate)
		{
			FWarpPointer(
				dpy, None, None, 0, 0, 0, 0, FinalX - x,
				FinalY - y);
		}
		if (IS_MAXIMIZED(fw))
		{
			fw->g.max.x += dx;
			fw->g.max.y += dy;
		}
		else
		{
			fw->g.normal.x += dx;
			fw->g.normal.y += dy;
		}
		update_absolute_geometry(fw);
		maximize_adjust_offset(fw);
		XFlush(dpy);
	}
	else /* icon window */
	{
		__move_icon(fw, FinalX, FinalY, x, y, do_animate, fWarp);
		XFlush(dpy);
	}
	focus_grab_buttons_on_layer(fw->layer);

	return;
}

void CMD_Move(F_CMD_ARGS)
{
	__move_window(F_PASS_ARGS, False, MOVE_NORMAL);

	return;
}

void CMD_AnimatedMove(F_CMD_ARGS)
{
	__move_window(F_PASS_ARGS, True, MOVE_NORMAL);

	return;
}

void CMD_MoveToPage(F_CMD_ARGS)
{
	__move_window(F_PASS_ARGS, False, MOVE_PAGE);

	return;
}

void CMD_MoveToScreen(F_CMD_ARGS)
{
	__move_window(F_PASS_ARGS, False, MOVE_SCREEN);

	return;
}

static void update_pos(
	int *dest_score, int *dest_pos, int current_pos, int requested_pos)
{
	int requested_score;

	requested_score = abs(current_pos - requested_pos);
	if (requested_score < *dest_score)
	{
		*dest_pos = requested_pos;
		*dest_score = requested_score;
	}

	return;
}

/* This function does the SnapAttraction stuff. It takes x and y coordinates
 * (*px and *py) and returns the snapped values. */
static void DoSnapAttract(
	FvwmWindow *fw, int Width, int Height, int *px, int *py)
{
	int nyt,nxl;
	rectangle self;
	int score_x;
	int score_y;

	nxl = -99999;
	nyt = -99999;
	score_x = 99999999;
	score_y = 99999999;
	self.x = *px;
	self.y = *py;
	self.width = Width;
	self.height = Height;
	{
		rectangle g;
		Bool rc;

		rc = get_visible_icon_title_geometry(fw, &g);
		if (rc == True)
		{
			self.height += g.height;
		}
	}

	/*
	 * Snap grid handling
	 */
	if (fw->snap_grid_x > 1 && nxl == -99999)
	{
		if (*px != *px / fw->snap_grid_x * fw->snap_grid_x)
		{
			nxl = (*px + ((*px >= 0) ?
				      fw->snap_grid_x : -fw->snap_grid_x) /
			       2) / fw->snap_grid_x * fw->snap_grid_x;
		}
	}
	if (fw->snap_grid_y > 1 && nyt == -99999)
	{
		if (*py != *py / fw->snap_grid_y * fw->snap_grid_y)
		{
			nyt = (*py + ((*py >= 0) ?
				      fw->snap_grid_y : -fw->snap_grid_y) /
			       2) / fw->snap_grid_y * fw->snap_grid_y;
		}
	}

	/*
	 * snap attraction
	 */
	/* snap to other windows or icons*/
	if (fw->snap_attraction.proximity > 0 &&
		(fw->snap_attraction.mode & (SNAP_ICONS | SNAP_WINDOWS | SNAP_SAME)))
	{
		FvwmWindow *tmp;
		int maskout = (SNAP_SCREEN | SNAP_SCREEN_WINDOWS |
				SNAP_SCREEN_ICONS | SNAP_SCREEN_ALL);

		for (tmp = Scr.FvwmRoot.next; tmp; tmp = tmp->next)
		{
			rectangle other;

			if (fw->Desk != tmp->Desk || fw == tmp)
			{
				continue;
			}
			/* check snapping type */
			switch (fw->snap_attraction.mode & ~(maskout))
			{
			case SNAP_WINDOWS:  /* we only snap windows */
				if (IS_ICONIFIED(tmp) || IS_ICONIFIED(fw))
				{
					continue;
				}
				break;
			case SNAP_ICONS:  /* we only snap icons */
				if (!IS_ICONIFIED(tmp) || !IS_ICONIFIED(fw))
				{
					continue;
				}
				break;
			case SNAP_SAME:  /* we don't snap unequal */
				if (IS_ICONIFIED(tmp) != IS_ICONIFIED(fw))
				{
					continue;
				}
				break;
			default:  /* All */
				/* NOOP */
				break;
			}
			/* get other window dimensions */
			get_visible_window_or_icon_geometry(tmp, &other);
			if (other.x >= Scr.MyDisplayWidth ||
			    other.x + other.width <= 0 ||
			    other.y >= Scr.MyDisplayHeight ||
			    other.y + other.height <= 0)
			{
				/* do not snap to windows that are not currently
				 * visible */
				continue;
			}
			/* snap horizontally */
			if (
				other.y + other.height > *py &&
				other.y < *py + self.height)
			{
				if (*px + self.width >= other.x &&
				    *px + self.width <
				    other.x + fw->snap_attraction.proximity)
				{
					update_pos(
						&score_x, &nxl, *px,
						other.x - self.width);
				}
				if (*px + self.width >=
				    other.x - fw->snap_attraction.proximity &&
				    *px + self.width < other.x)
				{
					update_pos(
						&score_x, &nxl, *px,
						other.x - self.width);
				}
				if (*px <= other.x + other.width &&
				    *px > other.x + other.width -
				    fw->snap_attraction.proximity)
				{
					update_pos(
						&score_x, &nxl, *px,
						other.x + other.width);
				}
				if (*px <= other.x + other.width +
				    fw->snap_attraction.proximity &&
				    *px > other.x + other.width)
				{
					update_pos(
						&score_x, &nxl, *px,
						other.x + other.width);
				}
			}
			/* snap vertically */
			if (
				other.x + other.width > *px &&
				other.x < *px + self.width)
			{
				if (*py + self.height >= other.y &&
				    *py + self.height < other.y +
				    fw->snap_attraction.proximity)
				{
					update_pos(
						&score_y, &nyt, *py,
						other.y - self.height);
				}
				if (*py + self.height >=
				    other.y - fw->snap_attraction.proximity &&
				    *py + self.height < other.y)
				{
					update_pos(
						&score_y, &nyt, *py,
						other.y - self.height);
				}
				if (*py <=
				    other.y + other.height &&
				    *py > other.y + other.height -
				    fw->snap_attraction.proximity)
				{
					update_pos(
						&score_y, &nyt, *py,
						other.y + other.height);
				}
				if (*py <= other.y + other.height +
				    fw->snap_attraction.proximity &&
				    *py > other.y + other.height)
				{
					update_pos(
						&score_y, &nyt, *py,
						other.y + other.height);
				}
			}
		} /* for */
	} /* snap to other windows */

	/* snap to screen egdes */
	if (fw->snap_attraction.proximity > 0 && (
			( fw->snap_attraction.mode & SNAP_SCREEN && (
				fw->snap_attraction.mode & SNAP_SAME ||
			( IS_ICONIFIED(fw) &&
				fw->snap_attraction.mode & SNAP_ICONS ) ||
			( !IS_ICONIFIED(fw) &&
				fw->snap_attraction.mode & SNAP_WINDOWS ))) ||
			( !IS_ICONIFIED(fw) &&
				fw->snap_attraction.mode & SNAP_SCREEN_WINDOWS ) ||
			( IS_ICONIFIED(fw) &&
				fw->snap_attraction.mode & SNAP_SCREEN_ICONS ) ||
			fw->snap_attraction.mode & SNAP_SCREEN_ALL ))
	{
		/* vertically */
		if (!(Scr.MyDisplayWidth < (*px) ||
		      (*px + self.width) < 0))
		{
			if (*py + self.height >=
			    Scr.MyDisplayHeight &&
			    *py + self.height <
			    Scr.MyDisplayHeight + fw->snap_attraction.proximity)
			{
				update_pos(
					&score_y, &nyt, *py,
					Scr.MyDisplayHeight - self.height);
			}
			if (*py + self.height >=
			    Scr.MyDisplayHeight - fw->snap_attraction.proximity &&
			    *py + self.height < Scr.MyDisplayHeight)
			{
				update_pos(
					&score_y, &nyt, *py,
					Scr.MyDisplayHeight - self.height);
			}
			if ((*py <= 0)&&(*py > - fw->snap_attraction.proximity))
			{
				update_pos(&score_y, &nyt, *py, 0);
			}
			if ((*py <=  fw->snap_attraction.proximity)&&(*py > 0))
			{
				update_pos(&score_y, &nyt, *py, 0);
			}
		} /* vertically */
		/* horizontally */
		if (!(Scr.MyDisplayHeight < (*py) ||
		      (*py + self.height) < 0))
		{
			if (*px + self.width >= Scr.MyDisplayWidth &&
			    *px + self.width <
			    Scr.MyDisplayWidth + fw->snap_attraction.proximity)
			{
				update_pos(
					&score_x, &nxl, *px,
					Scr.MyDisplayWidth - self.width);
			}
			if (*px + self.width >=
			    Scr.MyDisplayWidth - fw->snap_attraction.proximity &&
			    *px + self.width < Scr.MyDisplayWidth)
			{
				update_pos(
					&score_x, &nxl, *px,
					Scr.MyDisplayWidth - self.width);
			}
			if ((*px <= 0) &&
			    (*px > - fw->snap_attraction.proximity))
			{
				update_pos(&score_x, &nxl, *px, 0);
			}
			if ((*px <= fw->snap_attraction.proximity) &&
			    (*px > 0))
			{
				update_pos(&score_x, &nxl, *px, 0);
			}
		} /* horizontally */
	} /* snap to screen edges */

	if (nxl != -99999)
	{
		*px = nxl;
	}
	if (nyt != -99999)
	{
		*py = nyt;
	}

	/*
	 * Resist moving windows beyond the edge of the screen
	 */
	if (fw->edge_resistance_move > 0)
	{
		/* snap to right edge */
		if (
			*px + Width > Scr.MyDisplayWidth &&
			*px + Width < Scr.MyDisplayWidth +
			fw->edge_resistance_move)
		{
			*px = Scr.MyDisplayWidth - Width;
		}
		/* snap to left edge */
		else if ((*px < 0) && (*px > -fw->edge_resistance_move))
		{
			*px = 0;
		}
		/* snap to bottom edge */
		if (
			*py + Height > Scr.MyDisplayHeight &&
			*py + Height < Scr.MyDisplayHeight +
			fw->edge_resistance_move)
		{
			*py = Scr.MyDisplayHeight - Height;
		}
		/* snap to top edge */
		else if (*py < 0 && *py > -fw->edge_resistance_move)
		{
			*py = 0;
		}
	}
	/* Resist moving windows between xineramascreens */
	if (fw->edge_resistance_xinerama_move > 0 && FScreenIsEnabled())
	{
		int scr_x0, scr_y0;
		int scr_x1, scr_y1;
		Bool do_recalc_rectangle = False;

		FScreenGetResistanceRect(
			*px, *py, Width, Height, &scr_x0, &scr_y0, &scr_x1,
			&scr_y1);

		/* snap to right edge */
		if (scr_x1 < Scr.MyDisplayWidth &&
		    *px + Width >= scr_x1 && *px + Width <
		    scr_x1 + fw->edge_resistance_xinerama_move)
		{
			*px = scr_x1 - Width;
			do_recalc_rectangle = True;
		}
		/* snap to left edge */
		else if (
			scr_x0 > 0 &&
			*px <= scr_x0 && scr_x0 - *px <
			fw->edge_resistance_xinerama_move)
		{
			*px = scr_x0;
			do_recalc_rectangle = True;
		}
		if (do_recalc_rectangle)
		{
			/* Snapping in X direction can move the window off a
			 * screen.  Thus, it may no longer be necessary to snap
			 * in Y direction. */
			FScreenGetResistanceRect(
				*px, *py, Width, Height, &scr_x0, &scr_y0,
				&scr_x1, &scr_y1);
		}
		/* snap to bottom edge */
		if (scr_y1 < Scr.MyDisplayHeight &&
		    *py + Height >= scr_y1 && *py + Height <
		    scr_y1 + fw->edge_resistance_xinerama_move)
		{
			*py = scr_y1 - Height;
		}
		/* snap to top edge */
		else if (
			scr_y0 > 0 &&
			*py <= scr_y0 && scr_y0 - *py <
			fw->edge_resistance_xinerama_move)
		{
			*py = scr_y0;
		}
	}

	return;
}

/*
 *
 * Move the rubberband around, return with the new window location
 *
 * Returns True if the window has to be resized after the move.
 *
 */
Bool __move_loop(
	const exec_context_t *exc, int XOffset, int YOffset, int Width,
	int Height, int *FinalX, int *FinalY, Bool do_move_opaque, int cursor)
{
	extern Window bad_window;
	Bool is_finished = False;
	Bool is_aborted = False;
	int xl,xl2,yt,yt2,delta_x,delta_y,paged;
	unsigned int button_mask = 0;
	FvwmWindow fw_copy;
	int dx = Scr.EdgeScrollX ? Scr.EdgeScrollX : Scr.MyDisplayWidth;
	int dy = Scr.EdgeScrollY ? Scr.EdgeScrollY : Scr.MyDisplayHeight;
	const int vx = Scr.Vx;
	const int vy = Scr.Vy;
	int xl_orig = 0;
	int yt_orig = 0;
	int cnx = 0;
	int cny = 0;
	int x_virtual_offset = 0;
	int y_virtual_offset = 0;
	Bool sent_cn = False;
	Bool do_resize_too = False;
	int x_bak;
	int y_bak;
	Window move_w = None;
	int orig_icon_x = 0;
	int orig_icon_y = 0;
	Bool do_snap = True;
	Bool was_snapped = False;
	/* if Alt is initially pressed don't enable no-snap until Alt is
	 * released */
	Bool nosnap_enabled = False;
	/* Must not set placed by button if the event is a modified KeyEvent */
	Bool is_fake_event;
	FvwmWindow *fw = exc->w.fw;
	unsigned int draw_parts = PART_NONE;
	XEvent e;

	if (!GrabEm(cursor, GRAB_NORMAL))
	{
		XBell(dpy, 0);
		return False;
	}
	if (!IS_MAPPED(fw) && !IS_ICONIFIED(fw))
	{
		do_move_opaque = False;
	}
	bad_window = None;
	if (IS_ICONIFIED(fw))
	{
		if (FW_W_ICON_PIXMAP(fw) != None)
		{
			move_w = FW_W_ICON_PIXMAP(fw);
		}
		else if (FW_W_ICON_TITLE(fw) != None)
		{
			move_w = FW_W_ICON_TITLE(fw);
		}
	}
	else
	{
		move_w = FW_W_FRAME(fw);
	}
	if (
		!XGetGeometry(
			dpy, move_w, &JunkRoot, &x_bak, &y_bak,
			(unsigned int*)&JunkWidth, (unsigned int*)&JunkHeight,
			(unsigned int*)&JunkBW, (unsigned int*)&JunkDepth))
	{
		/* This is allright here since the window may not be mapped
		 * yet. */
	}

	if (IS_ICONIFIED(fw))
	{
		rectangle g;

		get_visible_icon_geometry(fw, &g);
		orig_icon_x = g.x;
		orig_icon_y = g.y;
	}

	/* make a copy of the fw structure for sending to the pager */
	memcpy(&fw_copy, fw, sizeof(FvwmWindow));
	/* prevent flicker when paging */
	SET_WINDOW_BEING_MOVED_OPAQUE(fw, do_move_opaque);

	if (FQueryPointer(
		    dpy, Scr.Root, &JunkRoot, &JunkChild, &xl, &yt,
		    &JunkX, &JunkY, &button_mask) == False)
	{
		/* pointer is on a different screen */
		xl = 0;
		yt = 0;
	}
	else
	{
		xl += XOffset;
		yt += YOffset;
	}
	button_mask &= DEFAULT_ALL_BUTTONS_MASK;
	xl_orig = xl;
	yt_orig = yt;

	/* draw initial outline */
	if (!IS_ICONIFIED(fw) &&
	    ((!do_move_opaque && !Scr.gs.do_emulate_mwm) || !IS_MAPPED(fw)))
	{
		draw_move_resize_grid(xl, yt, Width - 1, Height - 1);
	}

	if (move_w == FW_W_FRAME(fw) && do_move_opaque)
	{
		draw_parts = border_get_transparent_decorations_part(fw);
	}
	DisplayPosition(fw, exc->x.elast, xl, yt, True);

	memset(&e, 0, sizeof(e));

	/* Unset the placed by button mask.
	 * If the move is canceled this will remain as zero.
	 */
	fw->placed_by_button = 0;
	while (!is_finished && bad_window != FW_W(fw))
	{
		int rc = 0;
		int old_xl;
		int old_yt;

		old_xl = xl;
		old_yt = yt;
		/* wait until there is an interesting event */
		while (rc != -1 &&
		       (!FPending(dpy) ||
			!FCheckMaskEvent(
				dpy, ButtonPressMask | ButtonReleaseMask |
				KeyPressMask | PointerMotionMask |
				ButtonMotionMask | ExposureMask, &e)))
		{
			XEvent le;
			int x;
			int y;

			fev_get_last_event(&le);

			xl -= XOffset;
			yt -= YOffset;

			rc = HandlePaging(
				&le, dx, dy, &xl, &yt, &delta_x, &delta_y,
				False, False, True, fw->edge_delay_ms_move);

			/* Fake an event to force window reposition */
			if (delta_x)
			{
				x_virtual_offset = 0;
			}
			xl += XOffset;
			if (delta_y)
			{
				y_virtual_offset = 0;
			}
			yt += YOffset;
			if (do_snap)
			{
				DoSnapAttract(fw, Width, Height, &xl, &yt);
				was_snapped = True;
			}
			fev_make_null_event(&e, dpy);
			e.type = MotionNotify;
			e.xmotion.time = fev_get_evtime();
			if (FQueryPointer(
				    dpy, Scr.Root, &JunkRoot, &JunkChild, &x,
				    &y, &JunkX, &JunkY, &JunkMask) == True)
			{
				e.xmotion.x_root = x;
				e.xmotion.y_root = y;
			}
			else
			{
				/* pointer is on a different screen */
				e.xmotion.x_root = 0;
				e.xmotion.y_root = 0;
			}
			e.xmotion.state = JunkMask;
			e.xmotion.same_screen = True;
			break;
		}
		if (rc == -1)
		{
			/* block until an event arrives */
			/* dv (2004-07-01): With XFree 4.1.0.1, some Mouse
			 * events are not reported to fvwm when the pointer
			 * moves very fast and suddenly stops in the corner of
			 * the screen.  Handle EnterNotify/LeaveNotify events
			 * too to get an idea where the pointer might be. */
			FMaskEvent(
				dpy, ButtonPressMask | ButtonReleaseMask |
				KeyPressMask | PointerMotionMask |
				ButtonMotionMask | ExposureMask |
				EnterWindowMask | LeaveWindowMask, &e);
		}

		/* discard extra events before a logical release */
		if (e.type == MotionNotify ||
		    e.type == EnterNotify || e.type == LeaveNotify)
		{
			while (FPending(dpy) > 0 &&
			       FCheckMaskEvent(
				       dpy, ButtonMotionMask |
				       PointerMotionMask | ButtonPressMask |
				       ButtonRelease | KeyPressMask |
				       EnterWindowMask | LeaveWindowMask, &e))
			{
				if (e.type == ButtonPress ||
				    e.type == ButtonRelease ||
				    e.type == KeyPress)
				{
					break;
				}
			}
		}
		if (e.type == EnterNotify || e.type == LeaveNotify)
		{
			XEvent e2;
			int x;
			int y;

			/* Query the pointer to catch the latest information.
			 * This *is* necessary. */
			if (FQueryPointer(
				    dpy, Scr.Root, &JunkRoot, &JunkChild, &x,
				    &y, &JunkX, &JunkY, &JunkMask) == True)
			{
				fev_make_null_event(&e2, dpy);
				e2.type = MotionNotify;
				e2.xmotion.time = fev_get_evtime();
				e2.xmotion.x_root = x;
				e2.xmotion.y_root = y;
				e2.xmotion.state = JunkMask;
				e2.xmotion.same_screen = True;
				e = e2;
				fev_fake_event(&e);
			}
			else
			{
				/* pointer is on a different screen,
				 * ignore event */
			}
		}
		is_fake_event = False;
		/* Handle a limited number of key press events to allow
		 * mouseless operation */
		if (e.type == KeyPress)
		{
			Keyboard_shortcuts(
				&e, fw, &x_virtual_offset,
				&y_virtual_offset, ButtonRelease);

			is_fake_event = (e.type != KeyPress);
		}
		switch (e.type)
		{
		case KeyPress:
			if (!(e.xkey.state & Mod1Mask))
			{
				nosnap_enabled = True;
			}
			do_snap = nosnap_enabled &&
				(e.xkey.state & Mod1Mask) ? False : True;

			/* simple code to bag out of move - CKH */
			if (XLookupKeysym(&(e.xkey), 0) == XK_Escape)
			{
				if (!do_move_opaque)
				{
					switch_move_resize_grid(False);
				}
				if (!IS_ICONIFIED(fw))
				{
					if (do_move_opaque)
					{
						*FinalX = fw->g.frame.x;
						*FinalY = fw->g.frame.y;
					}
				}
				else
				{
					*FinalX = orig_icon_x;
					*FinalY = orig_icon_y;
				}
				is_aborted = True;
				is_finished = True;
			}
			break;
		case ButtonPress:
			if (e.xbutton.button <= NUMBER_OF_MOUSE_BUTTONS &&
			    ((Button1Mask << (e.xbutton.button - 1)) &
			     button_mask))
			{
				/* No new button was pressed, just a delayed
				 * event */
				break;
			}
			if (!IS_MAPPED(fw) &&
			    ((e.xbutton.button == 2 && !Scr.gs.do_emulate_mwm)
			     ||
			    (e.xbutton.button == 1 && Scr.gs.do_emulate_mwm &&
			     (e.xbutton.state & ShiftMask))))
			{
				do_resize_too = True;
				/* Fallthrough to button-release */
			}
			else if (!button_mask && e.xbutton.button <=
				 NUMBER_OF_EXTENDED_MOUSE_BUTTONS &&
				 e.xbutton.button > 0 &&
				 (move_interactive_finish_button_mask &
				  (1<<(e.xbutton.button-1))))
			{
				do_resize_too = False;
				break;
			}
			else if (button_mask &&  e.xbutton.button <=
				 NUMBER_OF_EXTENDED_MOUSE_BUTTONS &&
				 e.xbutton.button > 0 &&
				 (move_drag_finish_button_mask &
				  (1<<(e.xbutton.button-1))))
			{
				do_resize_too = False;
				/* Fallthrough to button-release */
			}
			else
			{
				/* Abort the move if
				 *  - the move started with a pressed button
				 *    and another button was pressed during the
				 *    operation
				 *  - Any button not in the
				 *    move_finish_button_mask is pressed
				 */
			  /*	if (button_mask) */
			  	/* - button_mask will always be set here.
				 *   only add an if if we want to be able to
				 *   place windows dragged by other means
				 *   than releasing the initial button.
				 */
				{
					if (!do_move_opaque)
					{
						switch_move_resize_grid(False);
					}
					if (!IS_ICONIFIED(fw))
					{
						*FinalX = fw->g.frame.x;
						*FinalY = fw->g.frame.y;
					}
					else
					{
						*FinalX = orig_icon_x;
						*FinalY = orig_icon_y;
					}
					is_aborted = True;
					is_finished = True;
				}
				break;
			}
		case ButtonRelease:
			if (!is_fake_event)
			{
				fw->placed_by_button = e.xbutton.button;
			}
			if (!do_move_opaque)
			{
				switch_move_resize_grid(False);
			}
			xl2 = e.xbutton.x_root + XOffset + x_virtual_offset;
			yt2 = e.xbutton.y_root + YOffset + y_virtual_offset;
			/* ignore the position of the button release if it was
			 * on a different page. */
			if (!(((xl <  0 && xl2 >= 0) ||
			       (xl >= 0 && xl2 <  0) ||
			       (yt <  0 && yt2 >= 0) ||
			       (yt >= 0 && yt2 <  0)) &&
			      (abs(xl - xl2) > Scr.MyDisplayWidth / 2 ||
			       abs(yt - yt2) > Scr.MyDisplayHeight / 2)))
			{
				xl = xl2;
				yt = yt2;
			}
			if (xl != xl_orig || yt != yt_orig || vx != Scr.Vx ||
			    vy != Scr.Vy || was_snapped)
			{
				/* only snap if the window actually moved! */
				if (do_snap)
				{
					DoSnapAttract(
						fw, Width, Height, &xl, &yt);
					was_snapped = True;
				}
			}

			*FinalX = xl;
			*FinalY = yt;

			is_finished = True;
			break;

		case MotionNotify:
			if (e.xmotion.same_screen == False)
			{
				continue;
			}
			if (!(e.xmotion.state & Mod1Mask))
			{
				nosnap_enabled = True;
			}
			do_snap = nosnap_enabled &&
				(e.xmotion.state & Mod1Mask) ? False : True;
			xl = e.xmotion.x_root;
			yt = e.xmotion.y_root;
			if (xl > 0 && xl < Scr.MyDisplayWidth - 1)
			{
				/* pointer was moved away from the left/right
				 * border with the mouse, reset the virtual x
				 * offset */
				x_virtual_offset = 0;
			}
			if (yt > 0 && yt < Scr.MyDisplayHeight - 1)
			{
				/* pointer was moved away from the top/bottom
				 * border with the mouse, reset the virtual y
				 * offset */
				y_virtual_offset = 0;
			}
			xl += XOffset + x_virtual_offset;
			yt += YOffset + y_virtual_offset;

			if (do_snap)
			{
				DoSnapAttract(fw, Width, Height, &xl, &yt);
				was_snapped = True;
			}

			/* check Paging request once and only once after
			 * outline redrawn redraw after paging if needed
			 * - mab */
			for (paged = 0; paged <= 1; paged++)
			{
				if (!do_move_opaque)
				{
					draw_move_resize_grid(
						xl, yt, Width - 1, Height - 1);
				}
				else
				{
					if (IS_ICONIFIED(fw))
					{
						set_icon_position(fw, xl, yt);
						move_icon_to_position(fw);
						broadcast_icon_geometry(
							fw, False);
					}
					else
					{
						XMoveWindow(
							dpy, FW_W_FRAME(fw),
							xl, yt);
					}
				}
				DisplayPosition(fw, &e, xl, yt, False);

				/* prevent window from lagging behind mouse
				 * when paging - mab */
				if (paged == 0)
				{
					XEvent le;

					xl = e.xmotion.x_root;
					yt = e.xmotion.y_root;
					fev_get_last_event(&le);
					HandlePaging(
						&le, dx, dy, &xl, &yt,
						&delta_x, &delta_y, False,
						False, False,
						fw->edge_delay_ms_move);
					if (delta_x)
					{
						x_virtual_offset = 0;
					}
					xl += XOffset;
					if (delta_y)
					{
						y_virtual_offset = 0;
					}
					yt += YOffset;
					if (do_snap)
					{
						DoSnapAttract(
							fw, Width, Height,
							&xl, &yt);
						was_snapped = True;
					}
					if (!delta_x && !delta_y)
					{
						/* break from while
						 * (paged <= 1) */
						break;
					}
				}
			}
			/* dv (13-Jan-2014): Without this XFlush the modules
			 * (and probably other windows too) sometimes get their
			 * expose only after the next motion step.  */
			XFlush(dpy);
			break;

		case Expose:
			if (!do_move_opaque)
			{
				/* must undraw the rubber band in case the
				 * event causes some drawing */
				switch_move_resize_grid(False);
			}
			dispatch_event(&e);
			if (!do_move_opaque)
			{
				draw_move_resize_grid(
					xl, yt, Width - 1, Height - 1);
			}
			break;

		default:
			/* cannot happen */
			break;
		} /* switch */
		xl += x_virtual_offset;
		yt += y_virtual_offset;
		if (do_move_opaque && !IS_ICONIFIED(fw) &&
		    !IS_SHADED(fw) && !Scr.bo.do_disable_configure_notify)
		{
			/* send configure notify event for windows that care
			 * about their location; don't send anything if
			 * position didn't change */
			if (!sent_cn || cnx != xl || cny != yt)
			{
				cnx = xl;
				cny = yt;
				sent_cn = True;
				SendConfigureNotify(
					fw, xl, yt, Width, Height, 0,
					False);
#ifdef FVWM_DEBUG_MSGS
				fvwm_msg(
					DBG, "frame_setup_window",
					"Sent ConfigureNotify (w %d, h %d)",
					Width, Height);
#endif
			}
		}
		if (do_move_opaque)
		{
			if (!IS_ICONIFIED(fw))
			{
				fw_copy.g.frame.x = xl;
				fw_copy.g.frame.y = yt;
			}
			if (xl != old_xl || yt != old_yt)
			{
				/* only do this with opaque moves, (i.e. the
				 * server is not grabbed) */
				if (draw_parts != PART_NONE)
				{
					border_draw_decorations(
						fw, draw_parts,
						((fw == get_focus_window())) ?
						True : False,
						True, CLEAR_ALL, NULL, NULL);
				}
				if (IS_TEAR_OFF_MENU(fw))
				{
					menu_redraw_transparent_tear_off_menu(
						fw, False);
				}
				BroadcastConfig(M_CONFIGURE_WINDOW, &fw_copy);
				FlushAllMessageQueues();
			}
		}
	} /* while (!is_finished) */

	if (!Scr.gs.do_hide_position_window)
	{
		XUnmapWindow(dpy,Scr.SizeWindow);
	}
	if (is_aborted || bad_window == FW_W(fw))
	{
		if (vx != Scr.Vx || vy != Scr.Vy)
		{
			MoveViewport(vx, vy, False);
		}
		if (is_aborted && do_move_opaque)
		{
			XMoveWindow(dpy, move_w, x_bak, y_bak);
			if (draw_parts != PART_NONE)
			{
				border_draw_decorations(
					fw, draw_parts,
					((fw == get_focus_window())) ?
					True : False,
					True, CLEAR_ALL, NULL, NULL);
			}
			menu_redraw_transparent_tear_off_menu(fw, False);
		}
		if (bad_window == FW_W(fw))
		{
			XUnmapWindow(dpy, move_w);
			border_undraw_decorations(fw);
			XBell(dpy, 0);
		}
	}
	if (!is_aborted && bad_window != FW_W(fw) && IS_ICONIFIED(fw))
	{
		SET_ICON_MOVED(fw, 1);
	}
	UngrabEm(GRAB_NORMAL);
	if (!do_resize_too)
	{
		/* Don't wait for buttons to come up when user is placing a new
		 * window and wants to resize it. */
		WaitForButtonsUp(True);
	}
	SET_WINDOW_BEING_MOVED_OPAQUE(fw, 0);
	bad_window = None;

	return do_resize_too;
}

void CMD_MoveThreshold(F_CMD_ARGS)
{
	int val = 0;

	if (GetIntegerArguments(action, NULL, &val, 1) < 1 || val < 0)
	{
		Scr.MoveThreshold = DEFAULT_MOVE_THRESHOLD;
	}
	else
	{
		Scr.MoveThreshold = val;
	}

	return;
}

void CMD_OpaqueMoveSize(F_CMD_ARGS)
{
	int val;

	if (GetIntegerArguments(action, NULL, &val, 1) < 1)
	{
		if (strncasecmp(action, "unlimited", 9) == 0)
		{
			Scr.OpaqueSize = -1;
		}
		else
		{
			Scr.OpaqueSize = DEFAULT_OPAQUE_MOVE_SIZE;
		}
	}
	else
	{
		Scr.OpaqueSize = val;
	}

	return;
}


static char *hide_options[] =
{
	"never",
	"move",
	"resize",
	NULL
};

void CMD_HideGeometryWindow(F_CMD_ARGS)
{
	char *token = PeekToken(action, NULL);

	Scr.gs.do_hide_position_window = 0;
	Scr.gs.do_hide_resize_window = 0;
	switch(GetTokenIndex(token, hide_options, 0, NULL))
	{
	case 0:
		break;
	case 1:
		Scr.gs.do_hide_position_window = 1;
		break;
	case 2:
		Scr.gs.do_hide_resize_window = 1;
		break;
	default:
		Scr.gs.do_hide_position_window = 1;
		Scr.gs.do_hide_resize_window = 1;
		break;
	}
	return;
}

void CMD_SnapAttraction(F_CMD_ARGS)
{
	char *cmd;
	size_t len;

	/* TA:  FIXME  xasprintf() */
	len = strlen(action);
	len += 99;
	cmd = xmalloc(len);
	sprintf(cmd, "Style * SnapAttraction %s", action);
	fvwm_msg(
		OLD, "CMD_SnapAttraction",
		"The command SnapAttraction is obsolete. Please use the"
		" following command instead:\n\n%s", cmd);
	execute_function(
		cond_rc, exc, cmd,
		FUNC_DONT_REPEAT | FUNC_DONT_EXPAND_COMMAND);
	free(cmd);

	return;
}

void CMD_SnapGrid(F_CMD_ARGS)
{
	char *cmd;
	size_t len;

	/* TA:  FIXME xasprintf() */
	len = strlen(action);
	len += 99;
	cmd = xmalloc(len);
	sprintf(cmd, "Style * SnapGrid %s", action);
	fvwm_msg(
		OLD, "CMD_SnapGrid",
		"The command SnapGrid is obsolete. Please use the following"
		" command instead:\n\n%s", cmd);
	execute_function(
		cond_rc, exc, cmd,
		FUNC_DONT_REPEAT | FUNC_DONT_EXPAND_COMMAND);
	free(cmd);

	return;
}

static Pixmap XorPixmap = None;

void CMD_XorValue(F_CMD_ARGS)
{
	int val;
	XGCValues gcv;
	unsigned long gcm;

	if (GetIntegerArguments(action, NULL, &val, 1) != 1)
	{
		val = 0;
	}

	PictureUseDefaultVisual();
	gcm = GCFunction|GCLineWidth|GCForeground|GCFillStyle|GCSubwindowMode;
	gcv.subwindow_mode = IncludeInferiors;
	gcv.function = GXxor;
	gcv.line_width = 1;
	/* use passed in value, or try to calculate appropriate value if 0 */
	/* ctwm method: */
	/*
	  gcv.foreground = (val1)?(val1):((((unsigned long) 1) <<
	  Scr.d_depth) - 1);
	*/
	/* Xlib programming manual suggestion: */
	gcv.foreground = (val)?
		(val):(PictureBlackPixel() ^ PictureWhitePixel());
	gcv.fill_style = FillSolid;
	gcv.subwindow_mode = IncludeInferiors;

	/* modify XorGC, only create once */
	if (Scr.XorGC)
	{
		XChangeGC(dpy, Scr.XorGC, gcm, &gcv);
	}
	else
	{
		Scr.XorGC = fvwmlib_XCreateGC(dpy, Scr.Root, gcm, &gcv);
	}

	/* free up XorPixmap if neccesary */
	if (XorPixmap != None) {
		XFreePixmap(dpy, XorPixmap);
		XorPixmap = None;
	}
	PictureUseFvwmVisual();

	return;
}


void CMD_XorPixmap(F_CMD_ARGS)
{
	char *PixmapName;
	FvwmPicture *xp;
	XGCValues gcv;
	unsigned long gcm;
	FvwmPictureAttributes fpa;

	action = GetNextToken(action, &PixmapName);
	if (PixmapName == NULL)
	{
		/* return to default value. */
		action = "0";
		CMD_XorValue(F_PASS_ARGS);
		return;
	}
	/* get the picture in the root visual, colorlimit is ignored because the
	 * pixels will be freed */
	fpa.mask = FPAM_NO_COLOR_LIMIT | FPAM_NO_ALPHA;
	PictureUseDefaultVisual();
	xp = PGetFvwmPicture(dpy, Scr.Root, NULL, PixmapName, fpa);
	if (xp == NULL)
	{
		fvwm_msg(ERR,"SetXORPixmap","Can't find pixmap %s", PixmapName);
		free(PixmapName);
		PictureUseFvwmVisual();
		return;
	}
	free(PixmapName);
	/* free up old pixmap */
	if (XorPixmap != None)
	{
		XFreePixmap(dpy, XorPixmap);
	}

	/* make a copy of the picture pixmap */
	XorPixmap = XCreatePixmap(dpy, Scr.Root, xp->width, xp->height, Pdepth);
	XCopyArea(dpy, xp->picture, XorPixmap, DefaultGC(dpy, Scr.screen), 0, 0,
		  xp->width, xp->height, 0, 0);
	/* destroy picture and free colors */
	PDestroyFvwmPicture(dpy, xp);
	PictureUseFvwmVisual();

	/* create Graphics context */
	gcm = GCFunction|GCLineWidth|GCTile|GCFillStyle|GCSubwindowMode;
	gcv.subwindow_mode = IncludeInferiors;
	gcv.function = GXxor;
	/* line width of 1 is necessary for Exceed servers */
	gcv.line_width = 1;
	gcv.tile = XorPixmap;
	gcv.fill_style = FillTiled;
	gcv.subwindow_mode = IncludeInferiors;
	/* modify XorGC, only create once */
	if (Scr.XorGC)
	{
		XChangeGC(dpy, Scr.XorGC, gcm, &gcv);
	}
	else
	{
		Scr.XorGC = fvwmlib_XCreateGC(dpy, Scr.Root, gcm, &gcv);
	}

	return;
}


/* ----------------------------- resizing code ----------------------------- */

/* Given a mouse location within a window context, return the direction the
 * window could be resized in, based on the window quadrant.  This is the same
 * as the quadrants drawn by the rubber-band, if "ResizeOpaque" has not been
 * set.
 */
static direction_t __resize_get_dir_from_resize_quadrant(
		int x_off, int y_off, int px, int py)
{
	direction_t dir = DIR_NONE;
	int tx;
	int ty;
	int dx;
	int dy;


	if (px < 0 || x_off < 0 || py < 0 || y_off < 0)
	{
		return dir;
	}

	/* Rough quadrants per window.  3x3. */
	tx = (x_off / 3) - 1;
	ty = (y_off / 3) - 1;

	dx = x_off - px;
	dy = y_off - py;

	if (px < tx)
	{
		/* Far left of window.  Quadrants of NW, W, SW. */
		if (py < ty)
		{
			/* North-West direction. */
			dir = DIR_NW;
		}
		else if (dy < ty)
		{
			/* South-West direction. */
			dir = DIR_SW;
		}
		else
		{
			/* West direction. */
			dir = DIR_W;
		}
	}
	else if (dx < tx)
	{
		/* Far right of window.  Quadrants NE, E, SE. */
		if (py < ty)
		{
			/* North-East direction. */
			dir = DIR_NE;
		}
		else if (dy < ty)
		{
			/* South-East direction */
			dir = DIR_SE;
		}
		else
		{
			/* East direction. */
			dir = DIR_E;
		}
	}
	else
	{
		if (py < ty)
		{
			/* North direction. */
			dir = DIR_N;
		}
		else if (dy < ty)
		{
			/* South direction. */
			dir = DIR_S;
		}
	}

	return dir;
}

static void __resize_get_dir_from_window(
	int *ret_xmotion, int *ret_ymotion, FvwmWindow *fw, Window context_w)
{
	if (context_w != Scr.Root && context_w != None)
	{
		if (context_w == FW_W_SIDE(fw, 0))   /* top */
		{
			*ret_ymotion = 1;
		}
		else if (context_w == FW_W_SIDE(fw, 1))  /* right */
		{
			*ret_xmotion = -1;
		}
		else if (context_w == FW_W_SIDE(fw, 2))  /* bottom */
		{
			*ret_ymotion = -1;
		}
		else if (context_w == FW_W_SIDE(fw, 3))  /* left */
		{
			*ret_xmotion = 1;
		}
		else if (context_w == FW_W_CORNER(fw, 0))  /* upper-left */
		{
			*ret_xmotion = 1;
			*ret_ymotion = 1;
		}
		else if (context_w == FW_W_CORNER(fw, 1))  /* upper-right */
		{
			*ret_xmotion = -1;
			*ret_ymotion = 1;
		}
		else if (context_w == FW_W_CORNER(fw, 2)) /* lower left */
		{
			*ret_xmotion = 1;
			*ret_ymotion = -1;
		}
		else if (context_w == FW_W_CORNER(fw, 3))  /* lower right */
		{
			*ret_xmotion = -1;
			*ret_ymotion = -1;
		}
	}

	return;
}

static void __resize_get_dir_proximity(
	int *ret_xmotion, int *ret_ymotion, FvwmWindow *fw, int x_off,
	int y_off, int px, int py, int *warp_x, int *warp_y, Bool find_nearest_border)
{
	int tx;
	int ty;
	direction_t dir;

	*warp_x = *warp_y = -1;

	if (px < 0 || x_off < 0 || py < 0 || y_off < 0)
	{
		return;
	}

	if (find_nearest_border == False)
	{
		tx = 0;
		ty = 0;

		tx = max(fw->boundary_width, tx);
		ty = max(fw->boundary_width, ty);

		if (px < tx)
		{
			*ret_xmotion = 1;
		}
		else if (x_off < tx)
		{
			*ret_xmotion = -1;
		}
		if (py < ty)
		{
			*ret_ymotion = 1;
		}
		else if (y_off < ty)
		{
			*ret_ymotion = -1;
		}

		return;
	}

	/* Get the direction from the quadrant the pointer is in. */
	dir = __resize_get_dir_from_resize_quadrant(
		x_off, y_off, px, py);

	switch (dir)
	{
	case DIR_NW:
		*ret_xmotion = 1;
		*ret_ymotion = 1;
		*warp_x = 0;
		*warp_y = 0;
		break;
	case DIR_SW:
		*ret_xmotion = 1;
		*ret_ymotion = -1;
		*warp_x = 0;
		*warp_y = (y_off - 1);
		break;
	case DIR_W:
		*ret_xmotion = 1;
		*warp_x = 0;
		*warp_y = (y_off / 2);
		break;
	case DIR_NE:
		*ret_xmotion = -1;
		*ret_ymotion = 1;
		*warp_x = (x_off - 1);
		*warp_y = 0;
		break;
	case DIR_SE:
		*ret_xmotion = -1;
		*ret_ymotion = -1;
		*warp_x = (x_off - 1);
		*warp_y = (y_off - 1);
		break;
	case DIR_E:
		*ret_xmotion = -1;
		*warp_x = (x_off - 1);
		*warp_y = (y_off / 2);
		break;
	case DIR_N:
		*ret_ymotion = 1;
		*warp_x = (x_off / 2);
		*warp_y = 0;
		break;
	case DIR_S:
		*ret_ymotion = -1;
		*warp_x = (x_off / 2);
		*warp_y = (y_off - 1);
		break;
	default:
		break;
	}

	return;
}

static void __resize_get_refpos(
	int *ret_x, int *ret_y, int xmotion, int ymotion, int w, int h,
	FvwmWindow *fw)
{
	if (xmotion > 0)
	{
		*ret_x = 0;
	}
	else if (xmotion < 0)
	{
		*ret_x = w - 1;
	}
	else
	{
		*ret_x = w / 2;
	}
	if (ymotion > 0)
	{
		*ret_y = 0;
	}
	else if (ymotion < 0)
	{
		*ret_y = h - 1;
	}
	else
	{
		*ret_y = h / 2;
	}

	return;
}

/* Procedure:
 *      __resize_step - move the rubberband around.  This is called for
 *                 each motion event when we are resizing
 *
 *  Inputs:
 *      x_root   - the X corrdinate in the root window
 *      y_root   - the Y corrdinate in the root window
 *      x_off    - x offset of pointer from border (input/output)
 *      y_off    - y offset of pointer from border (input/output)
 *      drag     - resize internal structure
 *      orig     - resize internal structure
 *      xmotionp - pointer to xmotion in resize_window
 *      ymotionp - pointer to ymotion in resize_window
 *
 */
static void __resize_step(
	const exec_context_t *exc, int x_root, int y_root, int *x_off,
	int *y_off, rectangle *drag, const rectangle *orig, int *xmotionp,
	int *ymotionp, Bool do_resize_opaque, Bool is_direction_fixed)
{
	int action = 0;
	int x2;
	int y2;
	int xdir;
	int ydir;

	x2 = x_root - *x_off;
	x_root += *x_off;
	if (is_direction_fixed == True && (*xmotionp != 0 || *ymotionp != 0))
	{
		xdir = *xmotionp;
	}
	else if (x2 <= orig->x ||
		 (*xmotionp == 1 && x2 < orig->x + orig->width - 1))
	{
		xdir = 1;
	}
	else if (x2 >= orig->x + orig->width - 1 ||
		 (*xmotionp == -1 && x2 > orig->x))
	{
		xdir = -1;
	}
	else
	{
		xdir = 0;
	}
	switch (xdir)
	{
	case 1:
		if (*xmotionp != 1)
		{
			*x_off = -*x_off;
			x_root = x2;
			*xmotionp = 1;
		}
		drag->x = x_root;
		drag->width = orig->x + orig->width - x_root;
		action = 1;
		break;
	case -1:
		if (*xmotionp != -1)
		{
			*x_off = -*x_off;
			x_root = x2;
			*xmotionp = -1;
		}
		drag->x = orig->x;
		drag->width = 1 + x_root - drag->x;
		action = 1;
		break;
	default:
		break;
	}
	y2 = y_root - *y_off;
	y_root += *y_off;
	if (is_direction_fixed == True && (*xmotionp != 0 || *ymotionp != 0))
	{
		ydir = *ymotionp;
	}
	else if (y2 <= orig->y ||
		 (*ymotionp == 1 && y2 < orig->y + orig->height - 1))
	{
		ydir = 1;
	}
	else if (y2 >= orig->y + orig->height - 1 ||
		 (*ymotionp == -1 && y2 > orig->y))
	{
		ydir = -1;
	}
	else
	{
		ydir = 0;
	}
	switch (ydir)
	{
	case 1:
		if (*ymotionp != 1)
		{
			*y_off = -*y_off;
			y_root = y2;
			*ymotionp = 1;
		}
		drag->y = y_root;
		drag->height = orig->y + orig->height - y_root;
		action = 1;
		break;
	case -1:
		if (*ymotionp != -1)
		{
			*y_off = -*y_off;
			y_root = y2;
			*ymotionp = -1;
		}
		drag->y = orig->y;
		drag->height = 1 + y_root - drag->y;
		action = 1;
		break;
	default:
		break;
	}

	if (action)
	{
		/* round up to nearest OK size to keep pointer inside
		 * rubberband */
		constrain_size(
			exc->w.fw, exc->x.elast, &drag->width, &drag->height,
			*xmotionp, *ymotionp, CS_ROUND_UP);
		if (*xmotionp == 1)
		{
			drag->x = orig->x + orig->width - drag->width;
		}
		if (*ymotionp == 1)
		{
			drag->y = orig->y + orig->height - drag->height;
		}
		if (!do_resize_opaque)
		{
			draw_move_resize_grid(
				drag->x, drag->y, drag->width - 1,
				drag->height - 1);
		}
		else
		{
			frame_setup_window(
				exc->w.fw, drag->x, drag->y, drag->width,
				drag->height, False);
		}
	}
	DisplaySize(exc->w.fw, exc->x.elast, drag->width, drag->height, False, False);

	return;
}

/* Starts a window resize operation */
static Bool __resize_window(F_CMD_ARGS)
{
	extern Window bad_window;
	FvwmWindow *fw = exc->w.fw;
	Bool is_finished = False, is_done = False, is_aborted = False;
	Bool do_send_cn = False;
	Bool do_resize_opaque;
	Bool do_warp_to_border;
	Bool is_direction_fixed;
	Bool automatic_border_direction;
	Bool detect_automatic_direction;
	Bool fButtonAbort = False;
	Bool fForceRedraw = False;
	Bool called_from_title = False;
	int x,y,delta_x,delta_y,stashed_x,stashed_y;
	Window ResizeWindow;
	int dx = Scr.EdgeScrollX ? Scr.EdgeScrollX : Scr.MyDisplayWidth;
	int dy = Scr.EdgeScrollY ? Scr.EdgeScrollY : Scr.MyDisplayHeight;
	const int vx = Scr.Vx;
	const int vy = Scr.Vy;
	int n;
	unsigned int button_mask = 0;
	rectangle sdrag;
	rectangle sorig;
	rectangle *drag = &sdrag;
	const rectangle *orig = &sorig;
	const window_g g_backup = fw->g;
	int ymotion = 0;
	int xmotion = 0;
	int was_maximized;
	unsigned edge_wrap_x;
	unsigned edge_wrap_y;
	int px;
	int py;
	int i;
	size_borders b;
	frame_move_resize_args mr_args = NULL;
	long evmask;
	XEvent ev;
	int ref_x;
	int ref_y;
	int x_off;
	int y_off;
	direction_t dir;
	int warp_x = 0;
	int warp_y = 0;

	bad_window = False;
	ResizeWindow = FW_W_FRAME(fw);
	if (fev_get_evpos_or_query(dpy, Scr.Root, exc->x.etrigger, &px, &py) ==
	    False ||
	    XTranslateCoordinates(
		    dpy, Scr.Root, ResizeWindow, px, py, &px, &py,
		    &JunkChild) == False)
	{
		/* pointer is on a different screen - that's okay here */
		px = 0;
		py = 0;
	}
	button_mask &= DEFAULT_ALL_BUTTONS_MASK;

	if (!is_function_allowed(F_RESIZE, NULL, fw, RQORIG_PROGRAM_US, True))
	{
		XBell(dpy, 0);
		return False;
	}

	if (IS_SHADED(fw) || !IS_MAPPED(fw))
	{
		do_resize_opaque = False;
		evmask = XEVMASK_RESIZE;
	}
	else
	{
		do_resize_opaque = DO_RESIZE_OPAQUE(fw);
		evmask = XEVMASK_RESIZE_OPAQUE;
	}

	/* no suffix = % of screen, 'p' = pixels, 'c' = increment units */
	if (IS_SHADED(fw))
	{
		get_unshaded_geometry(fw, drag);
	}
	else
	{
		drag->width = fw->g.frame.width;
		drag->height = fw->g.frame.height;
	}

	get_window_borders(fw, &b);
	n = GetResizeArguments(
		&action, fw->g.frame.x, fw->g.frame.y,
		fw->hints.base_width, fw->hints.base_height,
		fw->hints.width_inc, fw->hints.height_inc,
		&b, &(drag->width), &(drag->height),
		&dir, &is_direction_fixed, &do_warp_to_border,
		&automatic_border_direction, &detect_automatic_direction);

	if (n == 2)
	{
		rectangle new_g;

		/* size will be less or equal to requested */
		if (IS_SHADED(fw))
		{
			rectangle shaded_g;

			get_unshaded_geometry(fw, &new_g);
			SET_MAXIMIZED(fw, 0);
			constrain_size(
				fw, NULL, &drag->width, &drag->height, xmotion,
				ymotion, 0);
			gravity_resize(
				fw->hints.win_gravity, &new_g,
				drag->width - new_g.width,
				drag->height - new_g.height);
			fw->g.normal = new_g;
			get_shaded_geometry(fw, &shaded_g, &new_g);
			frame_setup_window(
				fw, shaded_g.x, shaded_g.y, shaded_g.width,
				shaded_g.height, False);
		}
		else
		{
			new_g = fw->g.frame;
			SET_MAXIMIZED(fw, 0);
			constrain_size(
				fw, NULL, &drag->width, &drag->height, xmotion,
				ymotion, 0);
			gravity_resize(
				fw->hints.win_gravity, &new_g,
				drag->width - new_g.width,
				drag->height - new_g.height);
			frame_setup_window(
				fw, new_g.x, new_g.y, drag->width,
				drag->height, False);
		}
		update_absolute_geometry(fw);
		maximize_adjust_offset(fw);
		ResizeWindow = None;
		return True;
	}

	was_maximized = IS_MAXIMIZED(fw);
	SET_MAXIMIZED(fw, 0);
	if (was_maximized)
	{
		/* must redraw the buttons now so that the 'maximize' button
		 * does not stay depressed. */
		border_draw_decorations(
			fw, PART_BUTTONS, (fw == Scr.Hilite), True, CLEAR_ALL,
			NULL, NULL);
	}

	if (Scr.bo.do_install_root_cmap)
	{
		InstallRootColormap();
	}
	else
	{
		InstallFvwmColormap();
	}
	if (!GrabEm(CRS_RESIZE, GRAB_NORMAL))
	{
		XBell(dpy, 0);
		return False;
	}

	/* handle problems with edge-wrapping while resizing */
	edge_wrap_x = Scr.flags.do_edge_wrap_x;
	edge_wrap_y = Scr.flags.do_edge_wrap_y;
	Scr.flags.do_edge_wrap_x = 0;
	Scr.flags.do_edge_wrap_y = 0;

	if (!do_resize_opaque)
	{
		MyXGrabServer(dpy);
	}
	if (!XGetGeometry(
		    dpy, (Drawable)ResizeWindow, &JunkRoot, &drag->x, &drag->y,
		    (unsigned int*)&drag->width, (unsigned int*)&drag->height,
		    (unsigned int*)&JunkBW, (unsigned int*)&JunkDepth))
	{
		UngrabEm(GRAB_NORMAL);
		if (!do_resize_opaque)
		{
			MyXUngrabServer(dpy);
		}
		return False;
	}
	if (IS_SHADED(fw))
	{
		SET_MAXIMIZED(fw, was_maximized);
		get_unshaded_geometry(fw, drag);
		SET_MAXIMIZED(fw, 0);
	}
	if (do_resize_opaque)
	{
		mr_args = frame_create_move_resize_args(
			fw, FRAME_MR_OPAQUE, &fw->g.frame, &fw->g.frame, 0,
			DIR_NONE);
	}
	else
	{
		Scr.flags.is_wire_frame_displayed = True;
	}
	MyXGrabKeyboard(dpy);

	sorig = *drag;
	ymotion = 0;
	xmotion = 0;

	/* pop up a resize dimensions window */
	if (!Scr.gs.do_hide_resize_window)
	{
		position_geometry_window(NULL);
		XMapRaised(dpy, Scr.SizeWindow);
	}
	DisplaySize(fw, exc->x.elast, orig->width, orig->height, True, True);

	if (dir == DIR_NONE && detect_automatic_direction == True)
	{
		dir = __resize_get_dir_from_resize_quadrant(
				orig->width, orig->height, px, py);
	}

	if (dir != DIR_NONE)
	{
		int grav;

		grav = gravity_dir_to_grav(dir);
		gravity_get_offsets(grav, &xmotion, &ymotion);
		xmotion = -xmotion;
		ymotion = -ymotion;
	}
	if (xmotion == 0 && ymotion == 0)
	{
		__resize_get_dir_from_window(&xmotion, &ymotion, fw, PressedW);
	}
	if (FW_W_TITLE(fw) != None && PressedW == FW_W_TITLE(fw))
	{
		/* title was pressed to start the resize */
		called_from_title = True;
	}
	else
	{
		for (i = NUMBER_OF_TITLE_BUTTONS; i--; )
		{
			/* see if the title button was pressed to that the
			 * resize */
			if (FW_W_BUTTON(fw, i) != None &&
			    FW_W_BUTTON(fw, i) == PressedW)
			{
				/* yes */
				called_from_title = True;
			}
		}
	}
	/* don't warp if the resize was triggered by a press somwhere on the
	 * title bar */
	if (PressedW != Scr.Root && xmotion == 0 && ymotion == 0 &&
	    !called_from_title)
	{
		__resize_get_dir_proximity(
			&xmotion, &ymotion, fw, orig->width,
			orig->height, px, py, &warp_x, &warp_y,
			automatic_border_direction);
		if (xmotion != 0 || ymotion != 0)
		{
			do_warp_to_border = True;
		}
	}
	if (!IS_SHADED(fw))
	{
		__resize_get_refpos(
			&ref_x, &ref_y, xmotion, ymotion, orig->width,
			orig->height, fw);
	}
	else
	{
		switch (SHADED_DIR(fw))
		{
		case DIR_N:
		case DIR_NW:
		case DIR_NE:
			if (ymotion == -1)
			{
				ymotion = 0;
			}
			break;
		case DIR_S:
		case DIR_SW:
		case DIR_SE:
			if (ymotion == 1)
			{
				ymotion = 0;
			}
			break;
		default:
			break;
		}
		switch (SHADED_DIR(fw))
		{
		case DIR_E:
		case DIR_NE:
		case DIR_SE:
			if (xmotion == 1)
			{
				xmotion = 0;
			}
			break;
		case DIR_W:
		case DIR_NW:
		case DIR_SW:
			if (xmotion == -1)
			{
				xmotion = 0;
			}
			break;
		default:
			break;
		}
		__resize_get_refpos(
			&ref_x, &ref_y, xmotion, ymotion, fw->g.frame.width,
			fw->g.frame.height, fw);
	}
	x_off = 0;
	y_off = 0;
	if (do_warp_to_border == True)
	{
		int dx;
		int dy;

		dx = (xmotion == 0) ? px : ref_x;
		dy = (ymotion == 0) ? py : ref_y;

		/* Warp the pointer to the closest border automatically? */
		if (automatic_border_direction == True &&
			(warp_x >=0 && warp_y >=0) &&
			!IS_SHADED(fw))
		{
			dx = warp_x;
			dy = warp_y;
		}

		/* warp the pointer to the border */
		FWarpPointer(
			dpy, None, ResizeWindow, 0, 0, 1, 1, dx, dy);
		XFlush(dpy);
	}
	else if (xmotion != 0 || ymotion != 0)
	{
		/* keep the distance between pointer and border */
		x_off = (xmotion == 0) ? 0 : ref_x - px;
		y_off = (ymotion == 0) ? 0 : ref_y - py;
	}
	else
	{
		/* wait until the pointer hits a border before making a
		 * decision about the resize direction */
	}

	/* draw the rubber-band window */
	if (!do_resize_opaque)
	{
		draw_move_resize_grid(
			drag->x, drag->y, drag->width - 1, drag->height - 1);
	}
	/* kick off resizing without requiring any motion if invoked with a key
	 * press */
	if (exc->x.elast->type == KeyPress)
	{
		int xo;
		int yo;

		if (FQueryPointer(
			    dpy, Scr.Root, &JunkRoot, &JunkChild, &stashed_x,
			    &stashed_y, &JunkX, &JunkY, &JunkMask) == False)
		{
			/* pointer is on a different screen */
			stashed_x = 0;
			stashed_y = 0;
		}
		xo = 0;
		yo = 0;
		__resize_step(
			exc, stashed_x, stashed_y, &xo, &yo, drag, orig,
			&xmotion, &ymotion, do_resize_opaque, True);
	}
	else
	{
		stashed_x = stashed_y = -1;
	}

	/* loop to resize */
	memset(&ev, 0, sizeof(ev));
	while (!is_finished && bad_window != FW_W(fw))
	{
		int rc = 0;
		int is_resized = False;

		/* block until there is an interesting event */
		while (rc != -1 &&
		       (!FPending(dpy) || !FCheckMaskEvent(dpy, evmask, &ev)))
		{
			rc = HandlePaging(
				&ev, dx, dy, &x, &y, &delta_x, &delta_y, False,
				False, True, fw->edge_delay_ms_resize);
			if (rc == 1)
			{
				/* Fake an event to force window reposition */
				ev.type = MotionNotify;
				ev.xmotion.time = fev_get_evtime();
				fForceRedraw = True;
				break;
			}
		}
		if (rc == -1)
		{
			FMaskEvent(
				dpy,
				evmask | EnterWindowMask | LeaveWindowMask,
				&ev);
		}
		if (ev.type == MotionNotify ||
		    ev.type == EnterNotify || ev.type == LeaveNotify)
		{
			/* discard any extra motion events before a release */
			/* dv (2004-07-01): With XFree 4.1.0.1, some Mouse
			 * events are not reported to fvwm when the pointer
			 * moves very fast and suddenly stops in the corner of
			 * the screen.  Handle EnterNotify/LeaveNotify events
			 * too to get an idea where the pointer might be. */
			while (
				FCheckMaskEvent(
					dpy, ButtonMotionMask |
					PointerMotionMask | ButtonReleaseMask |
					ButtonPressMask | EnterWindowMask |
					LeaveWindowMask, &ev) == True)
			{
				if (ev.type == ButtonRelease ||
				    ev.type == ButtonPress ||
				    ev.type == KeyPress)
				{
					break;
				}
			}
		}
		if (ev.type == EnterNotify || ev.type == LeaveNotify)
		{
			XEvent e2;
			int x;
			int y;

			/* Query the pointer to catch the latest information.
			 * This *is* necessary. */
			if (FQueryPointer(
				    dpy, Scr.Root, &JunkRoot, &JunkChild, &x,
				    &y, &JunkX, &JunkY, &JunkMask) == True)
			{
				/* Must NOT use button_mask here, or resize
				 * will not work with num lock */
				fev_make_null_event(&e2, dpy);
				e2.type = MotionNotify;
				e2.xmotion.time = fev_get_evtime();
				e2.xmotion.x_root = x;
				e2.xmotion.y_root = y;
				e2.xmotion.state = JunkMask;
				e2.xmotion.same_screen = True;
				ev = e2;
				fev_fake_event(&ev);
			}
			else
			{
				/* pointer is on a different screen,
				 * ignore event */
			}
		}

		is_done = False;
		/* Handle a limited number of key press events to allow
		 * mouseless operation */
		if (ev.type == KeyPress)
		{
			Keyboard_shortcuts(&ev, fw, NULL, NULL, ButtonRelease);
			if (ev.type == ButtonRelease)
			{
				do_send_cn = True;
			}
		}
		switch (ev.type)
		{
		case ButtonPress:
			is_done = True;
			if (ev.xbutton.button <= NUMBER_OF_MOUSE_BUTTONS &&
			    ((Button1Mask << (ev.xbutton.button - 1)) &
			     button_mask))
			{
				/* No new button was pressed, just a delayed
				 * event */
				break;
			}
			/* Abort the resize if
			 *  - the move started with a pressed button and
			 *    another button was pressed during the operation
			 *  - no button was started at the beginning and any
			 *    button except button 1 was pressed. */
			if (button_mask || (ev.xbutton.button != 1))
			{
				fButtonAbort = True;
				/* fall through */
			}
			else
			{
				is_finished = True;
				do_send_cn = True;
				break;
			}
		case KeyPress:
			/* simple code to bag out of move - CKH */
			if (fButtonAbort ||
			    XLookupKeysym(&ev.xkey, 0) == XK_Escape)
			{
				is_aborted = True;
				do_send_cn = True;
				is_finished = True;
			}
			is_done = True;
			break;

		case ButtonRelease:
			is_finished = True;
			is_done = True;
			break;

		case MotionNotify:
			if (ev.xmotion.same_screen == False)
			{
				continue;
			}
			if (!fForceRedraw)
			{
				x = ev.xmotion.x_root;
				y = ev.xmotion.y_root;
				/* resize before paging request to prevent
				 * resize from lagging * mouse - mab */
				__resize_step(
					exc, x, y, &x_off, &y_off, drag, orig,
					&xmotion, &ymotion, do_resize_opaque,
					is_direction_fixed);
				is_resized = True;
				/* need to move the viewport */
				HandlePaging(
					&ev, dx, dy, &x, &y, &delta_x,
					&delta_y, False, False, False,
					fw->edge_delay_ms_resize);
			}
			/* redraw outline if we paged - mab */
			if (delta_x != 0 || delta_y != 0)
			{
				sorig.x -= delta_x;
				sorig.y -= delta_y;
				drag->x -= delta_x;
				drag->y -= delta_y;

				__resize_step(
					exc, x, y, &x_off, &y_off, drag, orig,
					&xmotion, &ymotion, do_resize_opaque,
					is_direction_fixed);
				is_resized = True;
			}
			fForceRedraw = False;
			is_done = True;
			break;

		case PropertyNotify:
		{
			evh_args_t ea;
			exec_context_changes_t ecc;

			ecc.x.etrigger = &ev;
			ea.exc = exc_clone_context(exc, &ecc, ECC_ETRIGGER);
			HandlePropertyNotify(&ea);
			exc_destroy_context(ea.exc);
			is_done = True;
			break;
		}

		default:
			break;
		}
		if (!is_done)
		{
			if (!do_resize_opaque)
			{
				/* must undraw the rubber band in case the
				 * event causes some drawing */
				switch_move_resize_grid(False);
			}
			dispatch_event(&ev);
			if (!do_resize_opaque)
			{
				draw_move_resize_grid(
					drag->x, drag->y, drag->width - 1,
					drag->height - 1);
			}
		}
		else
		{
			if (do_resize_opaque)
			{
				/* only do this with opaque resizes, (i.e. the
				 * server is not grabbed) */
				if (is_resized == False)
				{
					BroadcastConfig(
						M_CONFIGURE_WINDOW, fw);
				}
				else
				{
					/* event already broadcast */
				}
				FlushAllMessageQueues();
			}
		}
	}

	/* erase the rubber-band */
	if (!do_resize_opaque)
	{
		switch_move_resize_grid(False);
	}
	/* pop down the size window */
	if (!Scr.gs.do_hide_resize_window)
	{
		XUnmapWindow(dpy, Scr.SizeWindow);
	}
	if (is_aborted || bad_window == FW_W(fw))
	{
		/* return pointer if aborted resize was invoked with key */
		if (stashed_x >= 0)
		{
			FWarpPointer(
				dpy, None, Scr.Root, 0, 0, 0, 0, stashed_x,
				stashed_y);
		}
		if (was_maximized)
		{
			/* since we aborted the resize, the window is still
			 * maximized */
			SET_MAXIMIZED(fw, 1);
		}
		if (do_resize_opaque)
		{
			int xo;
			int yo;
			rectangle g;

			xo = 0;
			yo = 0;
			xmotion = 1;
			ymotion = 1;
			g = sorig;
			__resize_step(
				exc, sorig.x, sorig.y, &xo, &yo, &g, orig,
				&xmotion, &ymotion, do_resize_opaque, True);
		}
		if (vx != Scr.Vx || vy != Scr.Vy)
		{
			MoveViewport(vx, vy, False);
		}
		/* restore all geometry-related info */
		fw->g = g_backup;
		if (bad_window == FW_W(fw))
		{
			XUnmapWindow(dpy, FW_W_FRAME(fw));
			border_undraw_decorations(fw);
			XBell(dpy, 0);
		}
	}
	else if (!is_aborted && bad_window != FW_W(fw))
	{
		rectangle new_g;

		/* size will be >= to requested */
		constrain_size(
			fw, exc->x.elast, &drag->width, &drag->height,
			xmotion, ymotion, CS_ROUND_UP);
		if (IS_SHADED(fw))
		{
			get_shaded_geometry(fw, &new_g, drag);
		}
		else
		{
			new_g = *drag;
		}
		if (do_resize_opaque)
		{
			frame_update_move_resize_args(mr_args, &new_g);
		}
		else
		{
			frame_setup_window(
				fw, new_g.x, new_g.y, new_g.width,
				new_g.height, False);
		}
		if (IS_SHADED(fw))
		{
			fw->g.normal.width = drag->width;
			fw->g.normal.height = drag->height;
		}
	}
	if (is_aborted && was_maximized)
	{
		/* force redraw */
		border_draw_decorations(
			fw, PART_BUTTONS, (fw == Scr.Hilite), True, CLEAR_ALL,
			NULL, NULL);
	}
	if (Scr.bo.do_install_root_cmap)
	{
		UninstallRootColormap();
	}
	else
	{
		UninstallFvwmColormap();
	}
	ResizeWindow = None;
	if (!do_resize_opaque)
	{
		int event_types[2] = { EnterNotify, LeaveNotify };

		/* Throw away some events that dont interest us right now. */
		discard_typed_events(2, event_types);
		Scr.flags.is_wire_frame_displayed = False;
		MyXUngrabServer(dpy);
	}
	if (mr_args != NULL)
	{
		frame_free_move_resize_args(fw, mr_args);
	}
	if (do_send_cn == True)
	{
		rectangle g;

		if (is_aborted)
		{
			g = sorig;
		}
		else
		{
			g = *drag;
		}
		SendConfigureNotify(fw, g.x, g.y, g.width, g.height, 0, True);
	}
	MyXUngrabKeyboard(dpy);
	WaitForButtonsUp(True);
	UngrabEm(GRAB_NORMAL);
	Scr.flags.do_edge_wrap_x = edge_wrap_x;
	Scr.flags.do_edge_wrap_y = edge_wrap_y;
	update_absolute_geometry(fw);
	maximize_adjust_offset(fw);
	if (is_aborted)
	{
		return False;
	}

	return True;
}

void CMD_Resize(F_CMD_ARGS)
{
	FvwmWindow *fw = exc->w.fw;

	if (IS_EWMH_FULLSCREEN(fw))
	{
		/* do not unmaximize ! */
		CMD_ResizeMaximize(F_PASS_ARGS);
		return;
	}

	__resize_window(F_PASS_ARGS);

	return;
}

/* ----------------------------- maximizing code --------------------------- */

Bool is_window_sticky_across_pages(FvwmWindow *fw)
{
	if (IS_STICKY_ACROSS_PAGES(fw) ||
	    (IS_ICONIFIED(fw) && IS_ICON_STICKY_ACROSS_PAGES(fw)))
	{
		return True;
	}
	else
	{
		return False;
	}
}

Bool is_window_sticky_across_desks(FvwmWindow *fw)
{
	if (IS_STICKY_ACROSS_DESKS(fw) ||
	    (IS_ICONIFIED(fw) && IS_ICON_STICKY_ACROSS_DESKS(fw)))
	{
		return True;
	}
	else
	{
		return False;
	}
}

static void move_sticky_window_to_same_page(
	int *x11, int *x12, int *y11, int *y12,
	int x21, int x22, int y21, int y22)
{
	/* make sure the x coordinate is on the same page as the reference
	 * window */
	if (*x11 >= x22)
	{
		while (*x11 >= x22)
		{
			*x11 -= Scr.MyDisplayWidth;
			*x12 -= Scr.MyDisplayWidth;
		}
	}
	else if (*x12 <= x21)
	{
		while (*x12 <= x21)
		{
			*x11 += Scr.MyDisplayWidth;
			*x12 += Scr.MyDisplayWidth;
		}
	}
	/* make sure the y coordinate is on the same page as the reference
	 * window */
	if (*y11 >= y22)
	{
		while (*y11 >= y22)
		{
			*y11 -= Scr.MyDisplayHeight;
			*y12 -= Scr.MyDisplayHeight;
		}
	}
	else if (*y12 <= y21)
	{
		while (*y12 <= y21)
		{
			*y11 += Scr.MyDisplayHeight;
			*y12 += Scr.MyDisplayHeight;
		}
	}

	return;
}

static void MaximizeHeight(
	FvwmWindow *win, int win_width, int win_x, int *win_height,
	int *win_y, Bool grow_up, Bool grow_down, int top_border,
	int bottom_border, int *layers)
{
	FvwmWindow *cwin;
	int x11, x12, x21, x22;
	int y11, y12, y21, y22;
	int new_y1, new_y2;
	rectangle g;
	Bool rc;

	x11 = win_x;             /* Start x */
	y11 = *win_y;            /* Start y */
	x12 = x11 + win_width;   /* End x   */
	y12 = y11 + *win_height; /* End y   */
	new_y1 = top_border;
	new_y2 = bottom_border;

	for (cwin = Scr.FvwmRoot.next; cwin; cwin = cwin->next)
	{
		if (cwin == win ||
		    (cwin->Desk != win->Desk &&
		     !is_window_sticky_across_desks(cwin)))
		{
			continue;
		}
		if ((layers[0] >= 0 && cwin->layer < layers[0]) ||
		    (layers[1] >= 0 && cwin->layer > layers[1]))
		{
			continue;
		}
		rc = get_visible_window_or_icon_geometry(cwin, &g);
		if (rc == False)
		{
			continue;
		}
		x21 = g.x;
		y21 = g.y;
		x22 = x21 + g.width;
		y22 = y21 + g.height;
		if (is_window_sticky_across_pages(cwin))
		{
			move_sticky_window_to_same_page(
				&x21, &x22, &new_y1, &new_y2, x11, x12, y11,
				y12);
		}

		/* Are they in the same X space? */
		if (!((x22 <= x11) || (x21 >= x12)))
		{
			if ((y22 <= y11) && (y22 >= new_y1))
			{
				new_y1 = y22;
			}
			else if ((y12 <= y21) && (new_y2 >= y21))
			{
				new_y2 = y21;
			}
		}
	}
	if (!grow_up)
	{
		new_y1 = y11;
	}
	if (!grow_down)
	{
		new_y2 = y12;
	}
	*win_height = new_y2 - new_y1;
	*win_y = new_y1;

	return;
}

static void MaximizeWidth(
	FvwmWindow *win, int *win_width, int *win_x, int win_height,
	int win_y, Bool grow_left, Bool grow_right, int left_border,
	int right_border, int *layers)
{
	FvwmWindow *cwin;
	int x11, x12, x21, x22;
	int y11, y12, y21, y22;
	int new_x1, new_x2;
	rectangle g;
	Bool rc;

	x11 = *win_x;            /* Start x */
	y11 = win_y;             /* Start y */
	x12 = x11 + *win_width;  /* End x   */
	y12 = y11 + win_height;  /* End y   */
	new_x1 = left_border;
	new_x2 = right_border;

	for (cwin = Scr.FvwmRoot.next; cwin; cwin = cwin->next)
	{
		if (cwin == win ||
		    (cwin->Desk != win->Desk &&
		     !is_window_sticky_across_desks(cwin)))
		{
			continue;
		}
		if ((layers[0] >= 0 && cwin->layer < layers[0]) ||
		    (layers[1] >= 0 && cwin->layer > layers[1]))
		{
			continue;
		}
		rc = get_visible_window_or_icon_geometry(cwin, &g);
		if (rc == False)
		{
			continue;
		}
		x21 = g.x;
		y21 = g.y;
		x22 = x21 + g.width;
		y22 = y21 + g.height;
		if (is_window_sticky_across_pages(cwin))
		{
			move_sticky_window_to_same_page(
				&new_x1, &new_x2, &y21, &y22, x11, x12, y11,
				y12);
		}

		/* Are they in the same Y space? */
		if (!((y22 <= y11) || (y21 >= y12)))
		{
			if ((x22 <= x11) && (x22 >= new_x1))
			{
				new_x1 = x22;
			}
			else if ((x12 <= x21) && (new_x2 >= x21))
			{
				new_x2 = x21;
			}
		}
	}
	if (!grow_left)
	{
		new_x1 = x11;
	}
	if (!grow_right)
	{
		new_x2 = x12;
	}
	*win_width  = new_x2 - new_x1;
	*win_x = new_x1;

	return;
}

static void unmaximize_fvwm_window(
	FvwmWindow *fw)
{
	rectangle new_g;

	SET_MAXIMIZED(fw, 0);
	if (IS_SHADED(fw))
	{
		get_shaded_geometry(fw, &new_g, &new_g);
	}

	/* We might be restoring a window's geometry coming out of fullscreen,
	 * which might be a maximized window, so ensure we use the correct
	 * geometry reference.
	 *
	 * If the window was not maximized, then we use the window's normal
	 * geometry.
	 */
	get_relative_geometry(&new_g, fw->fullscreen.was_maximized ?
			&fw->fullscreen.g.max : &fw->g.normal);

	if (fw->fullscreen.was_maximized)
	{
		/* If MWMButtons is in use, set the style of the button as
		 * marked as maximized.
		 */
		SET_MAXIMIZED(fw, 1);
	}

	if (IS_EWMH_FULLSCREEN(fw))
	{
		SET_EWMH_FULLSCREEN(fw, False);
		if (DO_EWMH_USE_STACKING_HINTS(fw))
		{
			new_layer(fw, fw->ewmh_normal_layer);
		}
	}

	/* TA:  Apply the decor change now, if the window we've just restored
	 * was maximized; the client frame and geometry will be updated as a
	 * result of this, so we correctly restore the window at this point.
	 */
	if (fw->fullscreen.was_maximized) {
		fw->fullscreen.was_maximized = 0;
		apply_decor_change(fw);
	}

	frame_setup_window(
		fw, new_g.x, new_g.y, new_g.width, new_g.height, True);
	border_draw_decorations(
		fw, PART_ALL, (Scr.Hilite == fw), True, CLEAR_ALL, NULL, NULL);

	if (fw->fullscreen.is_shaded)
	{
		execute_function_override_window(
			NULL, NULL, "WindowShade on", 0, fw);

		fw->fullscreen.is_shaded = 0;
	}

	if (fw->fullscreen.is_iconified) {
		execute_function_override_window(
			NULL, NULL, "Iconify on", 0, fw);
		fw->fullscreen.is_iconified = 0;
	}

	/* Since the window's geometry will have been constrained already when
	 * coming out of fullscreen, we can always call this; either it's a
	 * noop or the window will be correctly decorated in the case of the
	 * window being restored from fullscreen to a non-maximized state.
	 */
	apply_decor_change(fw);
	return;
}

static void maximize_fvwm_window(
	FvwmWindow *fw, rectangle *geometry)
{
	SET_MAXIMIZED(fw, 1);
	fw->g.max_defect.width = 0;
	fw->g.max_defect.height = 0;
	constrain_size(
		fw, NULL, &geometry->width, &geometry->height, 0, 0,
		CS_UPDATE_MAX_DEFECT);
	fw->g.max = *geometry;
	if (IS_SHADED(fw))
	{
		get_shaded_geometry(fw, geometry, &fw->g.max);
	}
	frame_setup_window(
		fw, geometry->x, geometry->y, geometry->width,
		geometry->height, True);
	border_draw_decorations(
		fw, PART_ALL, (Scr.Hilite == fw), True, CLEAR_ALL, NULL, NULL);
	update_absolute_geometry(fw);
	/* remember the offset between old and new position in case the
	 * maximized  window is moved more than the screen width/height. */
	fw->g.max_offset.x = fw->g.normal.x - fw->g.max.x;
	fw->g.max_offset.y = fw->g.normal.y - fw->g.max.y;
#if 0
fprintf(stderr,"%d %d %d %d, g.max_offset.x = %d, g.max_offset.y = %d, %d %d %d %d\n", fw->g.max.x, fw->g.max.y, fw->g.max.width, fw->g.max.height, fw->g.max_offset.x, fw->g.max_offset.y, fw->g.normal.x, fw->g.normal.y, fw->g.normal.width, fw->g.normal.height);
#endif

    return;
}

/*
 *
 *  Procedure:
 *      (Un)Maximize a window.
 *
 */
void CMD_Maximize(F_CMD_ARGS)
{
	int page_x, page_y;
	int val1, val2, val1_unit, val2_unit;
	int toggle;
	char *token;
	char *taction;
	Bool grow_up = False;
	Bool grow_down = False;
	Bool grow_left = False;
	Bool grow_right = False;
	Bool do_force_maximize = False;
	Bool do_forget = False;
	Bool is_screen_given = False;
	Bool ignore_working_area = False;
	Bool do_fullscreen = False;
	int layers[2] = { -1, -1 };
	Bool global_flag_parsed = False;
	int  scr_x, scr_y;
	int scr_w, scr_h;
	rectangle new_g;
	FvwmWindow *fw = exc->w.fw;

	if (
		!is_function_allowed(
			F_MAXIMIZE, NULL, fw, RQORIG_PROGRAM_US, False))
	{
		XBell(dpy, 0);
		return;
	}
	/* Check for "global" flag ("absolute" is for compatibility with E) */
	while (!global_flag_parsed)
	{
		token = PeekToken(action, &taction);
		if (!token)
		{
			global_flag_parsed = True;
		}
		else
		{
			if (StrEquals(token, "screen"))
			{
				int scr;

				is_screen_given = True;
				token = PeekToken(taction, &action);
				scr = FScreenGetScreenArgument(
					token, FSCREEN_SPEC_PRIMARY);
				FScreenGetScrRect(
					NULL, scr, &scr_x, &scr_y, &scr_w,
					&scr_h);
			}
			else if (StrEquals(token, "ewmhiwa"))
			{
				ignore_working_area = True;
				action = taction;
			}
			else if (StrEquals(token, "growonwindowlayer"))
			{
				layers[0] = fw->layer;
				layers[1] = fw->layer;
				action = taction;
			}
			else if (StrEquals(token, "growonlayers"))
			{
				int n;

				n = GetIntegerArguments(
					taction, &action, layers, 2);
				if (n != 2)
				{
					layers[0] = -1;
					layers[1] = -1;
				}
			}
			else if (StrEquals(token, "fullscreen"))
			{
				do_fullscreen = True;
				action = taction;
			}
			else
			{
				global_flag_parsed = True;
			}
		}
	}
	toggle = ParseToggleArgument(action, &action, -1, 0);
	if (do_fullscreen) {
		if (toggle == -1) {
			/* Flip-flop between fullscreen or not, if no toggle
			 * argument is given.
			 */
			toggle = (IS_EWMH_FULLSCREEN(fw) ? 0 : 1);
		}
		if (toggle == 1 && !IS_EWMH_FULLSCREEN(fw)) {
			EWMH_fullscreen(fw);
			return;
		}

		if (toggle == 0 && IS_EWMH_FULLSCREEN(fw)) {
			unmaximize_fvwm_window(fw);
			return;
		}
		return;
	} else {
		if (toggle == 0 && !IS_MAXIMIZED(fw))
		{
			return;
		}

		if (toggle == 1 && IS_MAXIMIZED(fw))
		{
			/* Fake that the window is not maximized. */
			do_force_maximize = True;
		}
	}

	/* find the new page and geometry */
	new_g.x = fw->g.frame.x;
	new_g.y = fw->g.frame.y;
	new_g.width = fw->g.frame.width;
	new_g.height = fw->g.frame.height;
	get_page_offset_check_visible(&page_x, &page_y, fw);

	/* Check if we should constrain rectangle to some Xinerama screen */
	if (!is_screen_given)
	{
		fscreen_scr_arg fscr;

		fscr.xypos.x = fw->g.frame.x + fw->g.frame.width  / 2 - page_x;
		fscr.xypos.y = fw->g.frame.y + fw->g.frame.height / 2 - page_y;
		FScreenGetScrRect(
			&fscr, FSCREEN_XYPOS, &scr_x, &scr_y, &scr_w, &scr_h);
	}

	if (!ignore_working_area)
	{
		EWMH_GetWorkAreaIntersection(
			fw, &scr_x, &scr_y, &scr_w, &scr_h,
			EWMH_MAXIMIZE_MODE(fw));
	}
#if 0
	fprintf(stderr, "%s: page=(%d,%d), scr=(%d,%d, %dx%d)\n", __FUNCTION__,
		page_x, page_y, scr_x, scr_y, scr_w, scr_h);
#endif

	/* parse first parameter */
	val1_unit = scr_w;
	token = PeekToken(action, &taction);
	if (token && StrEquals(token, "forget"))
	{
		if (!IS_MAXIMIZED(fw))
		{
			return;
		}
		do_forget = True;
		do_force_maximize = True;
	}
	else if (token && StrEquals(token, "grow"))
	{
		grow_left = True;
		grow_right = True;
		val1 = 100;
		val1_unit = scr_w;
	}
	else if (token && StrEquals(token, "growleft"))
	{
		grow_left = True;
		val1 = 100;
		val1_unit = scr_w;
	}
	else if (token && StrEquals(token, "growright"))
	{
		grow_right = True;
		val1 = 100;
		val1_unit = scr_w;
	}
	else
	{
		if (GetOnePercentArgument(token, &val1, &val1_unit) == 0)
		{
			val1 = 100;
			val1_unit = scr_w;
		}
		else if (val1 < 0)
		{
			/* handle negative offsets */
			if (val1_unit == scr_w)
			{
				val1 = 100 + val1;
			}
			else
			{
				val1 = scr_w + val1;
			}
		}
	}

	/* parse second parameter */
	val2_unit = scr_h;
	token = PeekToken(taction, NULL);
	if (do_forget == True)
	{
		/* nop */
	}
	else if (token && StrEquals(token, "grow"))
	{
		grow_up = True;
		grow_down = True;
		val2 = 100;
		val2_unit = scr_h;
	}
	else if (token && StrEquals(token, "growup"))
	{
		grow_up = True;
		val2 = 100;
		val2_unit = scr_h;
	}
	else if (token && StrEquals(token, "growdown"))
	{
		grow_down = True;
		val2 = 100;
		val2_unit = scr_h;
	}
	else
	{
		if (GetOnePercentArgument(token, &val2, &val2_unit) == 0)
		{
			val2 = 100;
			val2_unit = scr_h;
		}
		else if (val2 < 0)
		{
			/* handle negative offsets */
			if (val2_unit == scr_h)
			{
				val2 = 100 + val2;
			}
			else
			{
				val2 = scr_h + val2;
			}
		}
	}

#if 0
	fprintf(stderr, "%s: page=(%d,%d), scr=(%d,%d, %dx%d)\n", __FUNCTION__,
		page_x, page_y, scr_x, scr_y, scr_w, scr_h);
#endif

	if (do_forget == True)
	{
		fw->g.normal = fw->g.max;
		unmaximize_fvwm_window(fw);
	}
	else if (IS_MAXIMIZED(fw) && !do_force_maximize)
	{
		unmaximize_fvwm_window(fw);
	}
	else /* maximize */
	{
		/* handle command line arguments */
		if (grow_up || grow_down)
		{
			MaximizeHeight(
				fw, new_g.width, new_g.x, &new_g.height,
				&new_g.y, grow_up, grow_down, page_y + scr_y,
				page_y + scr_y + scr_h, layers);
		}
		else if (val2 > 0)
		{
			new_g.height = val2 * val2_unit / 100;
			new_g.y = page_y + scr_y;
		}
		if (grow_left || grow_right)
		{
			MaximizeWidth(
				fw, &new_g.width, &new_g.x, new_g.height,
				new_g.y, grow_left, grow_right,
				page_x + scr_x, page_x + scr_x + scr_w,
				layers);
		}
		else if (val1 >0)
		{
			new_g.width = val1 * val1_unit / 100;
			new_g.x = page_x + scr_x;
		}
		if (val1 == 0 && val2 == 0)
		{
			new_g.x = page_x + scr_x;
			new_g.y = page_y + scr_y;
			new_g.height = scr_h;
			new_g.width = scr_w;
		}
		/* now maximize it */
		maximize_fvwm_window(fw, &new_g);
	}
	EWMH_SetWMState(fw, False);

	return;
}

/*
 *
 * Same as CMD_Resize and CMD_ResizeMove, but the window ends up maximized
 * without touching the normal geometry.
 *
 */
void CMD_ResizeMaximize(F_CMD_ARGS)
{
	rectangle normal_g;
	rectangle max_g;
	Bool was_resized;
	FvwmWindow *fw = exc->w.fw;

	/* keep a copy of the old geometry */
	normal_g = fw->g.normal;
	/* resize the window normally */
	was_resized = __resize_window(F_PASS_ARGS);
	if (was_resized == True)
	{
		/* set the new geometry as the maximized geometry and restore
		 * the old normal geometry */
		max_g = fw->g.normal;
		max_g.x -= Scr.Vx;
		max_g.y -= Scr.Vy;
		fw->g.normal = normal_g;
		/* and mark it as maximized */
		maximize_fvwm_window(fw, &max_g);
	}
	EWMH_SetWMState(fw, False);

	return;
}

void CMD_ResizeMoveMaximize(F_CMD_ARGS)
{
	rectangle normal_g;
	rectangle max_g;
	Bool was_resized;
	FvwmWindow *fw = exc->w.fw;

	/* keep a copy of the old geometry */
	normal_g = fw->g.normal;
	/* resize the window normally */
	was_resized = resize_move_window(F_PASS_ARGS);
	if (was_resized == True)
	{
		/* set the new geometry as the maximized geometry and restore
		 * the old normal geometry */
		max_g = fw->g.normal;
		max_g.x -= Scr.Vx;
		max_g.y -= Scr.Vy;
		fw->g.normal = normal_g;
		/* and mark it as maximized */
		maximize_fvwm_window(fw, &max_g);
	}
	EWMH_SetWMState(fw, False);

	return;
}

/* ----------------------------- stick code -------------------------------- */

int stick_across_pages(F_CMD_ARGS, int toggle)
{
	FvwmWindow *fw = exc->w.fw;

	if ((toggle == 1 && IS_STICKY_ACROSS_PAGES(fw)) ||
	    (toggle == 0 && !IS_STICKY_ACROSS_PAGES(fw)))
	{
		return 0;
	}
	if (IS_STICKY_ACROSS_PAGES(fw))
	{
		SET_STICKY_ACROSS_PAGES(fw, 0);
	}
	else
	{
		if (!IsRectangleOnThisPage(&fw->g.frame, Scr.CurrentDesk))
		{
			action = "";
			__move_window(F_PASS_ARGS, False, MOVE_PAGE);
		}
		SET_STICKY_ACROSS_PAGES(fw, 1);
	}

	return 1;
}

int stick_across_desks(F_CMD_ARGS, int toggle)
{
	FvwmWindow *fw = exc->w.fw;

	if ((toggle == 1 && IS_STICKY_ACROSS_DESKS(fw)) ||
	    (toggle == 0 && !IS_STICKY_ACROSS_DESKS(fw)))
	{
		return 0;
	}

	if (IS_STICKY_ACROSS_DESKS(fw))
	{
		SET_STICKY_ACROSS_DESKS(fw, 0);
		fw->Desk = Scr.CurrentDesk;
	}
	else
	{
		if (fw->Desk != Scr.CurrentDesk)
		{
			do_move_window_to_desk(fw, Scr.CurrentDesk);
		}
		SET_STICKY_ACROSS_DESKS(fw, 1);
	}

	return 1;
}

static void __handle_stick_exit(
	FvwmWindow *fw, int do_not_draw, int do_silently)
{
	if (do_not_draw == 0)
	{
		border_draw_decorations(
			fw, PART_TITLE | PART_BUTTONS, (Scr.Hilite==fw), True,
			CLEAR_ALL, NULL, NULL);
	}
	if (!do_silently)
	{
		BroadcastConfig(M_CONFIGURE_WINDOW,fw);
		EWMH_SetWMState(fw, False);
		EWMH_SetWMDesktop(fw);
	}

	return;
}

void handle_stick_across_pages(
	F_CMD_ARGS, int toggle, int do_not_draw, int do_silently)
{
	FvwmWindow *fw = exc->w.fw;
	int did_change;

	did_change = stick_across_pages(F_PASS_ARGS, toggle);
	if (did_change)
	{
		__handle_stick_exit(fw, do_not_draw, do_silently);
	}

	return;
}

void handle_stick_across_desks(
	F_CMD_ARGS, int toggle, int do_not_draw, int do_silently)
{
	FvwmWindow *fw = exc->w.fw;
	int did_change;

	did_change = stick_across_desks(F_PASS_ARGS, toggle);
	if (did_change)
	{
		__handle_stick_exit(fw, do_not_draw, do_silently);
	}

	return;
}

void handle_stick(
	F_CMD_ARGS, int toggle_page, int toggle_desk, int do_not_draw,
	int do_silently)
{
	FvwmWindow *fw = exc->w.fw;
	int did_change;

	did_change = 0;
	did_change |= stick_across_desks(F_PASS_ARGS, toggle_desk);
	did_change |= stick_across_pages(F_PASS_ARGS, toggle_page);
	if (did_change)
	{
		__handle_stick_exit(fw, do_not_draw, do_silently);
	}

	return;
}

void CMD_Stick(F_CMD_ARGS)
{
	int toggle;

	toggle = ParseToggleArgument(action, &action, -1, 0);
	if (toggle == -1 && IS_STICKY_ACROSS_DESKS(exc->w.fw) !=
	    IS_STICKY_ACROSS_PAGES(exc->w.fw))
	{
		/* don't switch between only stickypage and only stickydesk.
		 * rather switch it off completely */
		toggle = 0;
	}
	handle_stick(F_PASS_ARGS, toggle, toggle, 0, 0);

	return;
}

void CMD_StickAcrossPages(F_CMD_ARGS)
{
	int toggle;

	toggle = ParseToggleArgument(action, &action, -1, 0);
	handle_stick_across_pages(F_PASS_ARGS, toggle, 0, 0);

	return;
}

void CMD_StickAcrossDesks(F_CMD_ARGS)
{
	int toggle;

	toggle = ParseToggleArgument(action, &action, -1, 0);
	handle_stick_across_desks(F_PASS_ARGS, toggle, 0, 0);

	return;
}
