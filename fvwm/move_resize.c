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
#include <stdbool.h>

#include "libs/fvwmlib.h"
#include "libs/Picture.h"
#include "libs/Grab.h"
#include "libs/Parse.h"
#include "libs/Graphics.h"
#include "libs/FEvent.h"
#include "fvwm.h"
#include "externs.h"
#include "cmdparser.h"
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
#include "externs.h"

#define DEBUG_CONFIGURENOTIFY 0

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

static void grow_to_closest_type(FvwmWindow *, rectangle *, rectangle, int *,
    int, bool);
static void set_geom_win_visible_val(char *, bool);
static bool set_geom_win_position_val(char *, int *, bool *, bool *);

/* ----- end of resize globals ----- */

/*
 *
 *  Procedure:
 *      draw_move_resize_grid - move a window outline
 *
 *  Inputs:
 *      root        - the window we are outlining
 *      geom        - outline rectangle
 *
 */
static int get_outline_rects(XRectangle *rects, rectangle geom)
{
	int i;
	int n;
	int m;

	n = 3;
	m = (geom.width - 5) / 2;
	if (m < n)
	{
		n = m;
	}
	m = (geom.height - 5) / 2;
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
		rects[i].x = geom.x + i;
		rects[i].y = geom.y + i;
		rects[i].width = geom.width - (i << 1);
		rects[i].height = geom.height - (i << 1);
	}
	if (geom.width - (n << 1) >= 5 && geom.height - (n << 1) >= 5)
	{
		if (geom.width - (n << 1) >= 10)
		{
			int off = (geom.width - (n << 1)) / 3 + n;
			rects[i].x = geom.x + off;
			rects[i].y = geom.y + n;
			rects[i].width = geom.width - (off << 1);
			rects[i].height = geom.height - (n << 1);
			i++;
		}
		if (geom.height - (n << 1) >= 10)
		{
			int off = (geom.height - (n << 1)) / 3 + n;
			rects[i].x = geom.x + n;
			rects[i].y = geom.y + off;
			rects[i].width = geom.width - (n << 1);
			rects[i].height = geom.height - (off << 1);
			i++;
		}
	}

	return i;
}

static struct
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

static void draw_move_resize_grid(rectangle geom)
{
	int nrects = 0;
	XRectangle rects[10];

	if (move_resize_grid.flags.is_enabled &&
	    geom.x == move_resize_grid.geom.x &&
	    geom.y == move_resize_grid.geom.y &&
	    geom.width == move_resize_grid.geom.width &&
	    geom.height == move_resize_grid.geom.height)
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
		nrects += get_outline_rects(&(rects[0]), move_resize_grid.geom);
	}
	if (geom.width && geom.height)
	{
		move_resize_grid.flags.is_enabled = 1;
		move_resize_grid.geom = geom;
		nrects += get_outline_rects(&(rects[nrects]), geom);
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
	rectangle zero_geom = { 0, 0, 0, 0 };

	if (state == False)
	{
		if (move_resize_grid.flags.is_enabled)
		{
			draw_move_resize_grid(zero_geom);
		}
		else
		{
			move_resize_grid.geom = zero_geom;
		}
	}
	else if (!move_resize_grid.flags.is_enabled)
	{
		if (move_resize_grid.geom.width &&
		    move_resize_grid.geom.height)
		{
			draw_move_resize_grid(move_resize_grid.geom);
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

/* Functions to shuffle windows to the closest boundary. */
static void grow_bound_to_next_monitor(
	FvwmWindow *fw, rectangle *bound, rectangle win_r, direction_t dir)
{
	position page;
	struct monitor *m;

	get_page_offset_check_visible(&page.x, &page.y, fw);
	bound->x -= page.x;
	bound->y -= page.y;
	win_r.x -= page.x;
	win_r.y -= page.y;

	TAILQ_FOREACH(m, &monitor_q, entry)
	{
		if (fw->m == m)
			continue;

		if (dir == DIR_N && m->si->y + m->si->h == fw->m->si->y &&
			win_r.x < m->si->x + m->si->w &&
			win_r.x + win_r.width > m->si->x)
		{
			bound->y = m->si->y + m->ewmhc.BaseStrut.top;
			bound->height = win_r.y + win_r.height - bound->y;
		}
		else if (dir == DIR_E && m->si->x == fw->m->si->x +
			fw->m->si->w &&	win_r.y < m->si->y + m->si->h &&
				win_r.y + win_r.height > m->si->y)
		{
			bound->width = m->si->x + m->si->w - bound->x -
				m->ewmhc.BaseStrut.right;
		}
		else if (dir == DIR_S && m->si->y == fw->m->si->y +
			fw->m->si->h &&	win_r.x < m->si->x + m->si->w &&
			win_r.x + win_r.width > m->si->x)
		{
			bound->height = m->si->y + m->si->h - bound->y -
				m->ewmhc.BaseStrut.bottom;
		}
		else if (dir == DIR_W && m->si->x + m->si->w == fw->m->si->x &&
			win_r.y < m->si->y + m->si->h &&
			win_r.y + win_r.height > m->si->y)
		{
			bound->x = m->si->x + m->ewmhc.BaseStrut.left;
			bound->width = win_r.x + win_r.width - bound->x;
		}
	}
	bound->x += page.x;
	bound->y += page.y;
}
static void shuffle_win_to_closest(
	FvwmWindow *fw, char **action, position *pFinal, Bool *fWarp)
{
	direction_t dir;
	rectangle cwin, bound;
	char *naction, *token = NULL;
	position page;
	int n;
	int snap = SNAP_NONE;
	int layers[2] = { -1, -1 };

	cwin = fw->g.frame;
	get_page_offset_check_visible(&page.x, &page.y, fw);

	token = PeekToken(*action, &naction);
	/* Get flags */
	while (token)
	{
		if (StrEquals(token, "snap"))
		{
			*action = naction;
			token = PeekToken(*action, &naction);
			if (StrEquals(token, "windows"))
				snap = SNAP_WINDOWS;
			else if (StrEquals(token, "icons"))
				snap = SNAP_ICONS;
			else if (StrEquals(token, "same"))
				snap = SNAP_SAME;
		}
		else if (StrEquals(token, "layers"))
		{
			*action = naction;
			n = GetIntegerArguments(*action, &naction, layers, 2);
			if (n != 2)
			{
				layers[0] = -1;
				layers[1] = -1;
			}
		}
		else if (StrEquals(token, "Warp") && fWarp != NULL)
		{
			*fWarp = true;
		}
		else
		{
			break;
		}
		*action = naction;
		token = PeekToken(*action, &naction);

	}

	/* Get direction(s) */
	while (token)
	{
		dir = gravity_parse_dir_argument(
				*action, &naction, DIR_NONE);

		switch (dir)
		{
		case DIR_N:
			bound.x = cwin.x;
			bound.y = fw->m->si->y + page.y;
			if (cwin.y - bound.y > fw->m->ewmhc.BaseStrut.top)
				bound.y += fw->m->ewmhc.BaseStrut.top;
			bound.width = cwin.width;
			bound.height = cwin.y + cwin.height - bound.y;
			if (cwin.y <= bound.y)
			{
				grow_bound_to_next_monitor(
					fw, &bound, cwin, DIR_N);
			}
			grow_to_closest_type(fw, &cwin, bound, layers,
				snap, false);
			cwin.height = fw->g.frame.height;
			break;
		case DIR_E:
			bound.x = cwin.x;
			bound.y = cwin.y;
			bound.width = fw->m->si->x + fw->m->si->w -
				bound.x + page.x;
			if (bound.x + bound.width - cwin.x - cwin.width >
				fw->m->ewmhc.BaseStrut.right)
			{
				bound.width -= fw->m->ewmhc.BaseStrut.right;
			}
			bound.height = cwin.height;
			if (cwin.x + cwin.width >= bound.x + bound.width)
			{
				grow_bound_to_next_monitor(
					fw, &bound, cwin, DIR_E);
			}
			grow_to_closest_type(fw, &cwin, bound, layers,
				snap, false);
			cwin.x = cwin.x + cwin.width - fw->g.frame.width;
			cwin.width = fw->g.frame.width;
			break;
		case DIR_S:
			bound.x = cwin.x;
			bound.y = cwin.y;
			bound.width = cwin.width;
			bound.height = fw->m->si->y + fw->m->si->h -
				bound.y + page.y;
			if (bound.y + bound.height - cwin.y - cwin.height >
				fw->m->ewmhc.BaseStrut.bottom)
			{
				bound.height -= fw->m->ewmhc.BaseStrut.bottom;
			}
			if (cwin.y + cwin.height >= bound.y + bound.height)
			{
				grow_bound_to_next_monitor(
					fw, &bound, cwin, DIR_S);
			}
			grow_to_closest_type(fw, &cwin, bound, layers,
				snap, false);
			cwin.y = cwin.y + cwin.height -	fw->g.frame.height;
			cwin.height = fw->g.frame.height;
			break;
		case DIR_W:
			bound.x = fw->m->si->x + page.x;
			if (cwin.x - bound.x > fw->m->ewmhc.BaseStrut.left)
				bound.x += fw->m->ewmhc.BaseStrut.left;
			bound.y = cwin.y;
			bound.width = cwin.y + cwin.width - bound.x;
			bound.height = cwin.height;
			bound.height = cwin.y + cwin.height - bound.y;
			if (cwin.x <= bound.x)
			{
				grow_bound_to_next_monitor(
					fw, &bound, cwin, DIR_W);
			}
			grow_to_closest_type(fw, &cwin, bound, layers,
				snap, false);
			cwin.width = fw->g.frame.width;
			break;
		case DIR_NONE:
			/* No direction found, need to move to next token */
			token = PeekToken(*action, &naction);
			break;
		default:
			break;
		}
		*action = naction;
		token = PeekToken(*action, &naction);
	}
	pFinal->x = cwin.x;
	pFinal->y = cwin.y;
}

static int get_shift(int val, float factor)
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
	int pos_change = 0;
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
			pos_change += get_shift(val, f);
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
		pos_change += get_shift(val, f);
	}
	final_pos += pos_change;
	if (final_pos > 32767 || final_pos < -32768)
	{
		fvwm_debug(
			__func__, "new position is out of range: %d",
			final_pos);

		return 0;
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
int GetMoveArguments(FvwmWindow *fw,
	char **paction, size_rect s, position *pFinal,
	Bool *fWarp, Bool *fPointer, Bool fKeep)
{
	char *s1 = NULL;
	char *s2 = NULL;
	char *token = NULL;
	char *action;
	char *naction;
	position scr_pos = { 0, 0 };
	int scr_w = monitor_get_all_widths();
	int scr_h = monitor_get_all_heights();
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
	if (s1 && StrEquals(s1, "shuffle"))
	{
		free(s1);
		shuffle_win_to_closest(fw, &action, pFinal, fWarp);
		*paction = action;
		return 2;

	}
	if (s1 && StrEquals(s1, "screen"))
	{
		fscreen_scr_arg parg;
		parg.mouse_ev = NULL;

		free(s1);
		token = PeekToken(action, &action);
		parg.name = token;

		/* When being asked to move a window to coordinates which are
		 * relative to a given screen, don't assume to use the
		 * screen's working area, as the coordinates given are not
		 * relative to that.
		 */
		use_working_area = False;

		FScreenGetScrRect(
			&parg, FSCREEN_BY_NAME, &scr_pos.x, &scr_pos.y,
			&scr_w, &scr_h);
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
			NULL, &scr_pos.x, &scr_pos.y, &scr_w, &scr_h,
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
				s1, pFinal->x, s.width, &(pFinal->x),
				(float)scr_w / 100, scr_w, scr_pos.x, True))
		{
			retval++;
		}
		if (fKeep == True && StrEquals(s2, "keep"))
		{
			retval++;
		}
		else if (
			GetOnePositionArgument(
				s2, pFinal->y, s.height, &(pFinal->y),
				(float)scr_h / 100, scr_h, scr_pos.y, False))
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
		while (pFinal->x < 0)
		{
			pFinal->x = monitor_get_all_widths() + pFinal->x;
		}
		while (pFinal->y < 0)
		{
			pFinal->y = monitor_get_all_heights() + pFinal->y;
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
	int size_change;
	int new_size;

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
		size_change = 0;
	}
	else if (sscanf(arg,"w-%d",&val) == 1)
	{
		size_change = -(int)(val * factor + 0.5);
	}
	else if (sscanf(arg,"w+%d",&val) == 1 || sscanf(arg,"w%d",&val) == 1)
	{
		size_change = (int)(val * factor + 0.5);
	}
	else if (sscanf(arg,"-%d",&val) == 1)
	{
		size_change =
			scr_size - (int)(val * factor + 0.5) + add_size -
			*ret_size;
	}
	else if (sscanf(arg,"+%d",&val) == 1 || sscanf(arg,"%d",&val) == 1)
	{
		size_change =
			(int)(val * factor + 0.5) + add_size + add_base_size -
			*ret_size;
	}
	else
	{
		return 0;
	}
	new_size = *ret_size + size_change;
	if (new_size < 0)
	{
		new_size = 0;
	}
	else if (new_size > 65535)
	{
		fvwm_debug(__func__, "new size is too big: %d", new_size);
		return 0;
	}
	*ret_size = new_size;

	return 1;
}

static int GetResizeArguments(
	FvwmWindow *fw,
	char **paction, position p, size_rect base_size, size_rect inc_size,
	size_borders *sb, size_rect *pFinalSize,
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
	size_rect add_size;
	int has_frame_option;
	struct monitor	*m = NULL;

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
		position pn = {
			p.x + pFinalSize->width - 1,
			p.y + pFinalSize->height - 1
		};
		size_rect sz = { 0, 0 };

		n = GetMoveArguments(fw, &naction, sz, &pn, NULL, NULL, True);
		if (n < 2)
		{
			return 0;
		}
		pFinalSize->width = pn.x - p.x + 1;
		pFinalSize->height = pn.y - p.y + 1;
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
				if (
					tmp_token != NULL && StrEquals(
						tmp_token, "automatic"))
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
		add_size.width = 0;
		add_size.height = 0;
	}
	else
	{
		add_size.width = sb->total_size.width;
		add_size.height = sb->total_size.height;
	}
	s1 = NULL;
	if (token != NULL)
	{
		s1 = fxstrdup(token);
	}
	naction = GetNextToken(naction, &s2);
	if (!s2)
	{
		free(s1);
		return 0;
	}
	*paction = naction;

	m = fw->m;
	n = 0;
	n += ParseOneResizeArgument(
		s1, monitor_get_all_widths(),
		m->Desktops->ewmh_working_area.width,
		m->Desktops->ewmh_dyn_working_area.width,
		base_size.width, inc_size.width,
		add_size.width, &(pFinalSize->width));
	n += ParseOneResizeArgument(
		s2, monitor_get_all_heights(),
		m->Desktops->ewmh_working_area.height,
		m->Desktops->ewmh_dyn_working_area.height,
		base_size.height, inc_size.height,
		add_size.height, &(pFinalSize->height));

	free(s1);
	free(s2);

	if (n < 2)
	{
		n = 0;
	}

	return n;
}

static int GetResizeMoveArguments(
	FvwmWindow *fw,
	char **paction, size_rect base_size, size_rect inc_size,
	size_borders *sb, position *pFinalPos, size_rect *pFinalSize,
	Bool *fWarp, Bool *fPointer)
{
	char *action = *paction;
	direction_t dir;
	Bool dummy;

	if (!paction)
	{
		return 0;
	}

	if (GetResizeArguments(
		    fw, &action, *pFinalPos, base_size, inc_size,
		    sb, pFinalSize, &dir, &dummy, &dummy, &dummy,
		    &dummy) < 2)
	{
		return 0;
	}
	if (GetMoveArguments(
		    fw, &action, *pFinalSize, pFinalPos, fWarp, NULL, True) < 2)
	{
		return 0;
	}
	*paction = action;

	return 4;
}

/* Positions the SizeWindow */
static void position_geometry_window(const XEvent *eventp)
{
	position p;
	size_rect sgn = { 1, 1 };
	fscreen_scr_t screen = FSCREEN_CURRENT;
	fscreen_scr_arg fscr;

	/* If we're being called without having been told which screen to use
	 * explicitly, then use the current screen.  This emulates the
	 * behaviour whereby the geometry window follows the pointer.
	 */
	if (Scr.SizeWindow.m == NULL) {
		fscr.mouse_ev = (XEvent *)eventp;
		screen = FSCREEN_CURRENT;
		fscr.name = NULL;
	} else {
		fscr.mouse_ev = (XEvent *)NULL;
		screen = FSCREEN_BY_NAME;
		fscr.name = Scr.SizeWindow.m->si->name;
	}

	if (Scr.SizeWindow.is_configured) {
		rectangle scr_g;
		FScreenGetScrRect(
			&fscr, screen, &scr_g.x, &scr_g.y,
			&scr_g.width, &scr_g.height);

		if (Scr.SizeWindow.xneg)
			sgn.width = -1;
		p.x = scr_g.x + sgn.width * (Scr.SizeWindow.x * (scr_g.width -
			sizew_g.width)) / 100;
		if (!Scr.SizeWindow.xrel)
			p.x = scr_g.x + sgn.width * Scr.SizeWindow.x;
		if (Scr.SizeWindow.xneg)
			p.x += scr_g.width - sizew_g.width;

		if (Scr.SizeWindow.yneg)
			sgn.height = -1;
		p.y = scr_g.y + sgn.height * (Scr.SizeWindow.y * (scr_g.height -
			sizew_g.height)) / 100;
		if (!Scr.SizeWindow.yrel)
			p.y = scr_g.y + sgn.height * Scr.SizeWindow.y;
		if (Scr.SizeWindow.yneg)
			p.y += scr_g.height - sizew_g.height;
	} else {
		FScreenGetScrRect(&fscr, screen, &p.x, &p.y, NULL, NULL);
	}

	if (p.x != sizew_g.x || p.y != sizew_g.y)
	{
		switch_move_resize_grid(False);
		XMoveWindow(dpy, Scr.SizeWindow.win, p.x, p.y);
		switch_move_resize_grid(True);
		sizew_g.x = p.x;
		sizew_g.y = p.y;
	}
}

void resize_geometry_window(void)
{
	size_rect s;
	int cset = Scr.DefaultColorset;

	if (Scr.SizeWindow.cset >= 0)
		cset = Scr.SizeWindow.cset;

	Scr.SizeWindow.StringWidth =
		FlocaleTextWidth(Scr.DefaultFont, GEOMETRY_WINDOW_STRING,
				 sizeof(GEOMETRY_WINDOW_STRING) - 1);
	s.width = Scr.SizeWindow.StringWidth + 2 * GEOMETRY_WINDOW_BW;
	s.height = Scr.DefaultFont->height + 2 * GEOMETRY_WINDOW_BW;
	if (s.width != sizew_g.width || s.height != sizew_g.height)
	{
		XResizeWindow(dpy, Scr.SizeWindow.win, s.width, s.height);
		sizew_g.width = s.width;
		sizew_g.height = s.height;
	}
	if (cset >= 0)
	{
		SetWindowBackground(
			dpy, Scr.SizeWindow.win, s.width, s.height,
			&Colorset[cset], Pdepth, Scr.StdGC, False);
	}
	else
	{
		XSetWindowBackground(dpy, Scr.SizeWindow.win, Scr.StdBack);
	}

	return;
}

static void display_string(Bool Init, char *str)
{
	static GC our_relief_gc = None;
	static GC our_shadow_gc = None;
	GC reliefGC;
	GC shadowGC;
	int offset;
	FlocaleWinString fstr;

	memset(&fstr, 0, sizeof(fstr));

	if (Scr.SizeWindow.cset >= 0)
	{
		fstr.colorset = &Colorset[Scr.SizeWindow.cset];
		fstr.flags.has_colorset = True;
	}
	else if (Scr.DefaultColorset >= 0)
	{
		fstr.colorset = &Colorset[Scr.DefaultColorset];
		fstr.flags.has_colorset = True;
	}

	if (Init)
	{
		XClearWindow(dpy,Scr.SizeWindow.win);
		if (our_relief_gc != None)
		{
			XFreeGC(dpy, our_relief_gc);
			our_relief_gc = None;
		}
		if (our_shadow_gc != None)
		{
			XFreeGC(dpy, our_shadow_gc);
			our_shadow_gc = None;
		}
		if (fstr.flags.has_colorset)
		{
			XGCValues gcv;

			gcv.foreground = fstr.colorset->hilite;
			our_relief_gc = fvwmlib_XCreateGC(
				dpy, Scr.NoFocusWin, GCForeground, &gcv);
			gcv.foreground = fstr.colorset->shadow;
			our_shadow_gc = fvwmlib_XCreateGC(
				dpy, Scr.NoFocusWin, GCForeground, &gcv);
		}
	}
	else
	{
		/* just clear indside the relief lines to reduce flicker */
		XClearArea(
			dpy, Scr.SizeWindow.win, GEOMETRY_WINDOW_BW,
			GEOMETRY_WINDOW_BW, Scr.SizeWindow.StringWidth,
			Scr.DefaultFont->height, False);
	}
	reliefGC = (our_relief_gc != None) ? our_relief_gc : Scr.StdReliefGC;
	shadowGC = (our_shadow_gc != None) ? our_shadow_gc : Scr.StdShadowGC;

	if (Pdepth >= 2)
	{
		RelieveRectangle(
			dpy, Scr.SizeWindow.win, 0, 0,
			Scr.SizeWindow.StringWidth + GEOMETRY_WINDOW_BW * 2 - 1,
			Scr.DefaultFont->height + GEOMETRY_WINDOW_BW*2 - 1,
			reliefGC, shadowGC, GEOMETRY_WINDOW_BW);
	}
	offset = (Scr.SizeWindow.StringWidth -
		  FlocaleTextWidth(Scr.DefaultFont, str, strlen(str))) / 2;
	offset += GEOMETRY_WINDOW_BW;

	fstr.str = str;
	fstr.win = Scr.SizeWindow.win;
	fstr.gc = Scr.StdGC;
	fstr.x = offset;
	fstr.y = Scr.DefaultFont->ascent + GEOMETRY_WINDOW_BW;
	FlocaleDrawString(dpy, Scr.DefaultFont, &fstr, 0);

}

/*
 *
 *  Procedure:
 *      DisplayPosition - display the position in the dimensions window
 *
 *  Inputs:
 *      tmp_win - the current fvwm window
 *      p       - position of the window
 *
 */

static void DisplayPosition(
	const FvwmWindow *tmp_win, const XEvent *eventp, position p, int Init)
{
	char str[100];
	fscreen_scr_arg fscr;

	if (Scr.gs.do_hide_position_window)
	{
		return;
	}
	position_geometry_window(eventp);
	/* Translate x,y into local screen coordinates. */
	fscr.xypos.x = p.x;
	fscr.xypos.y = p.y;
	FScreenTranslateCoordinates(NULL, FSCREEN_GLOBAL, &fscr,
			FSCREEN_XYPOS, &p.x, &p.y);
	(void)sprintf(str, GEOMETRY_WINDOW_POS_STRING, p.x, p.y);
	display_string(Init, str);

	return;
}

static void DisplaySize(
	const FvwmWindow *tmp_win, const XEvent *eventp, size_rect size,
	Bool Init, Bool resetLast)
{
	char str[100];
	size_rect d;
	size_borders b;
	static size_rect last_size = { 0, 0 };

	if (Scr.gs.do_hide_resize_window)
	{
		return;
	}
	position_geometry_window(eventp);
	if (resetLast)
	{
		last_size.width = 0;
		last_size.height = 0;
	}
	if (last_size.width == size.width && last_size.height == size.height)
	{
		return;
	}
	last_size.width = size.width;
	last_size.height = size.height;

	get_window_borders(tmp_win, &b);
	d.height = size.height - b.total_size.height;
	d.width = size.width - b.total_size.width;
	d.width -= tmp_win->hints.base_width;
	d.height -= tmp_win->hints.base_height;
	d.width /= tmp_win->hints.width_inc;
	d.height /= tmp_win->hints.height_inc;

	(void)sprintf(str, GEOMETRY_WINDOW_SIZE_STRING, d.width, d.height);
	display_string(Init, str);

	return;
}

static Bool resize_move_window(F_CMD_ARGS)
{
	position final_pos = { 0, 0 };
	size_rect final_size = { 0, 0 };
	int n;
	position p;
	Bool fWarp = False;
	Bool fPointer = False;
	position d;
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
		    dpy, w, &JunkRoot, &p.x, &p.y,
		    (unsigned int*)&final_size.width,
		    (unsigned int*)&final_size.height,
		    (unsigned int*)&JunkBW, (unsigned int*)&JunkDepth))
	{
		XBell(dpy, 0);
		return False;
	}

	final_pos = p;

	get_window_borders(fw, &b);
	{
		size_rect base = {
			fw->hints.base_width, fw->hints.base_height
		};
		size_rect inc = {
			fw->hints.width_inc, fw->hints.height_inc,
		};

		n = GetResizeMoveArguments(
			fw, &action, base, inc,
			&b, &final_pos, &final_size, &fWarp, &fPointer);
	}
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
	d.x = final_pos.x - fw->g.frame.x;
	d.y = final_pos.y - fw->g.frame.y;
	/* size will be less or equal to requested */
	constrain_size(
		fw, NULL, &final_size.width, &final_size.height, 0, 0, 0);
	if (IS_SHADED(fw))
	{
		frame_setup_window(
			fw, final_pos.x, final_pos.y,
			final_size.width, final_size.height, False);
	}
	else
	{
		frame_setup_window(
			fw, final_pos.x, final_pos.y,
			final_size.width, final_size.height, True);
	}
	if (fWarp)
	{
		FWarpPointer(
			dpy, None, None, 0, 0, 0, 0,
			final_pos.x - p.x, final_pos.y - p.y);
	}
	if (IS_MAXIMIZED(fw))
	{
		fw->g.max.x += d.x;
		fw->g.max.y += d.y;
	}
	else
	{
		fw->g.normal.x += d.x;
		fw->g.normal.y += d.y;
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
	Window *win, const exec_context_t *exc, position *pFinal,
	Bool do_start_at_pointer)
{
	rectangle ppos;
	position wpos_orig;
	position offset;
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
			    dpy, Scr.Root, &JunkRoot, &JunkChild, &ppos.x,
			    &ppos.y, &JunkX, &JunkY, &JunkMask) == False)
		{
			/* pointer is on a different screen */
			ppos.x = 0;
			ppos.y = 0;
		}
	}
	else
	{
		/* Although a move is usually done with a button depressed we
		 * have to check for ButtonRelease too since the event may be
		 * faked. */
		fev_get_evpos_or_query(
			dpy, Scr.Root, exc->x.elast, &ppos.x, &ppos.y);
	}

	MyXGrabServer(dpy);
	if (!XGetGeometry(
		    dpy, w, &JunkRoot, &wpos_orig.x, &wpos_orig.y,
		    (unsigned int*)&ppos.width, (unsigned int*)&ppos.height,
		    (unsigned int*)&JunkBW, (unsigned int*)&JunkDepth))
	{
		MyXUngrabServer(dpy);
		return;
	}
	MyXGrabKeyboard(dpy);
	if (do_start_at_pointer)
	{
		wpos_orig.x = ppos.x;
		wpos_orig.y = ppos.y;
	}

	if (IS_ICONIFIED(exc->w.fw))
	{
		do_move_opaque = True;
	}
	else if (IS_MAPPED(exc->w.fw))
	{
		float areapct;

		areapct = 100.0;
		areapct *=
			((float)ppos.width / (float) monitor_get_all_widths());
		areapct *=
			((float)ppos.height /
			 (float) monitor_get_all_heights());
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

	offset.x = wpos_orig.x - ppos.x;
	offset.y = wpos_orig.y - ppos.y;
	if (!Scr.gs.do_hide_position_window)
	{
		resize_geometry_window();
		position_geometry_window(NULL);
		XMapRaised(dpy,Scr.SizeWindow.win);
	}
	{
		size_rect sz = { ppos.width, ppos.height };

		move_loop(exc, offset, sz, pFinal, do_move_opaque);
	}
	if (!Scr.gs.do_hide_position_window)
	{
		XUnmapWindow(dpy,Scr.SizeWindow.win);
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
	FvwmWindow *fw, Window w, position start, position end,
	Bool fWarpPointerToo, int cmsDelay, float *ppctMovement,
	MenuRepaintTransparentParameters *pmrtp)
{
	position pointer;
	position current;
	position last;
	position delta;
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

	if (start.x < 0 || start.y < 0)
	{
		if (
			!XGetGeometry(
				dpy, w, &JunkRoot, &current.x, &current.y,
				(unsigned int*)&JunkWidth,
				(unsigned int*)&JunkHeight,
				(unsigned int*)&JunkBW,
				(unsigned int*)&JunkDepth))
		{
			XBell(dpy, 0);
			return;
		}
		if (start.x < 0)
		{
			start.x = current.x;
		}
		if (start.y < 0)
		{
			start.y = current.y;
		}
	}

	delta.x = end.x - start.x;
	delta.y = end.y - start.y;
	last.x = start.x;
	last.y = start.y;

	if (delta.x == 0 && delta.y == 0)
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
		current.x = start.x + delta.x * (*ppctMovement);
		current.y = start.y + delta.y * (*ppctMovement);
		if (last.x == current.x && last.y == current.y)
		{
			/* don't waste time in the same spot */
			continue;
		}
		if (pmrtp != NULL)
		{
			update_transparent_menu_bg(
				pmrtp, last.x, last.y, current.x, current.y,
				end.x, end.y);
		}
		XMoveWindow(dpy,w,current.x,current.y);
		if (pmrtp != NULL)
		{
			repaint_transparent_menu(
				pmrtp, first,
				current.x, current.y, end.x, end.y, True);
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
				    &JunkX, &JunkY, &pointer.x, &pointer.y,
				    &JunkMask) == False)
			{
				/* pointer is on a different screen */
				pointer = current;
			}
			else
			{
				pointer.x += current.x - last.x;
				pointer.y += current.y - last.y;
			}
			FWarpPointer(
				dpy, None, Scr.Root, 0, 0, 0, 0, pointer.x,
				pointer.y);
		}
		if (fw && !IS_SHADED(fw) && !Scr.bo.do_disable_configure_notify)
		{
			/* send configure notify event for windows that care
			 * about their location */
			SendConfigureNotify(
				fw, current.x, current.y,
				fw->g.frame.width,
				fw->g.frame.height, 0, False);
			if (DEBUG_CONFIGURENOTIFY)
			{
				fvwm_debug(
					__func__,
					"Sent ConfigureNotify"
					" (w == %d, h == %d)",
					fw->g.frame.width,
					fw->g.frame.height);
			}
		}
		XFlush(dpy);
		if (fw)
		{
			fw->g.frame.x = current.x;
			fw->g.frame.y = current.y;
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
					pmrtp, last.x, last.y,
					current.x, current.y, end.x, end.y);
			}
			XMoveWindow(dpy, w, end.x, end.y);
			if (pmrtp != NULL)
			{
				repaint_transparent_menu(
					pmrtp, first,
					end.x, end.y, end.x, end.y, True);
			}
			break;
		}
		last.x = current.x;
		last.y = current.y;
		first = False;
	} while (*ppctMovement != 1.0 && ppctMovement++);
	MyXUngrabKeyboard(dpy);
	XFlush(dpy);

	return;
}

/* used for moving menus, not a client window */
void AnimatedMoveOfWindow(
	Window w, position start, position end,
	Bool fWarpPointerToo, int cmsDelay, float *ppctMovement,
	MenuRepaintTransparentParameters *pmrtp)
{
	AnimatedMoveAnyWindow(
		NULL, w, start, end, fWarpPointerToo,
		cmsDelay, ppctMovement, pmrtp);

	return;
}

/* used for moving client windows */
void AnimatedMoveFvwmWindow(
	FvwmWindow *fw, Window w, position start, position end,
	Bool fWarpPointerToo, int cmsDelay, float *ppctMovement)
{
	AnimatedMoveAnyWindow(
		fw, w, start, end, fWarpPointerToo,
		cmsDelay, ppctMovement, NULL);

	return;
}

int placement_binding(int button, KeySym keysym, int modifier, char *action)
{
	if (keysym != 0)
	{
		/* fixme */
		fvwm_debug(__func__,
			   "sorry, placement keybindings not allowed. yet.");
		return 1;
	}
	if (modifier != 0)
	{
		/* fixme */
		fvwm_debug(__func__,
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
		fvwm_debug(__func__,
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
	FvwmWindow *fw, position new, position old,
	Bool do_move_animated, Bool do_warp_pointer)
{
	rectangle gt;
	rectangle gp;
	Bool has_icon_title;
	Bool has_icon_picture;
	Window tw;
	position t;

	set_icon_position(fw, new.x, new.y);
	broadcast_icon_geometry(fw, False);
	has_icon_title = get_visible_icon_title_geometry(fw, &gt);
	has_icon_picture = get_visible_icon_picture_geometry(fw, &gp);
	if (has_icon_picture)
	{
		tw = FW_W_ICON_PIXMAP(fw);
		t.x = gp.x;
		t.y = gp.y;
	}
	else if (has_icon_title)
	{
		tw = FW_W_ICON_TITLE(fw);
		t.x = gt.x;
		t.y = gt.y;
	}
	else
	{
		return;
	}
	if (do_move_animated)
	{
		position start = { -1, -1 };

		AnimatedMoveOfWindow(
			tw, start, t, do_warp_pointer, -1, NULL, NULL);
		do_warp_pointer = 0;
	}
	if (has_icon_title)
	{
		XMoveWindow(dpy, FW_W_ICON_TITLE(fw), gt.x, gt.y);
	}
	if (has_icon_picture)
	{
		struct monitor	*m = fw->m;
		XMoveWindow(dpy, FW_W_ICON_PIXMAP(fw), gp.x, gp.y);
		if (fw->Desk == m->virtual_scr.CurrentDesk)
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
		FWarpPointer(
			dpy, None, None, 0, 0, 0, 0,
			new.x - old.x, new.y - old.y);
	}

	return;
}

static void __move_window(F_CMD_ARGS, Bool do_animate, int mode)
{
	position final = { 0, 0 };
	int n;
	position p;
	size_rect sz;
	position page;
	Bool fWarp = False;
	Bool fPointer = False;
	position d;
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
			dpy, w, &JunkRoot, &p.x, &p.y, (unsigned int*)&sz.width,
			(unsigned int*)&sz.height, (unsigned int*)&JunkBW,
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

		struct monitor *m;

		do_animate = False;
		r.x = p.x;
		r.y = p.y;
		r.width = sz.width;
		r.height = sz.height;
		get_absolute_geometry(fw, &t, &r);
		get_page_offset_rectangle(fw, &page.x, &page.y, &t);

		if (!get_page_arguments(fw, action, &page.x, &page.y, &m))
		{
			m = fw->m;
			page.x = m->virtual_scr.Vx;
			page.y = m->virtual_scr.Vy;
		}
		/* It's possible that we've received a command such as:
		 *
		 * MoveToPage HDMI2 0 1
		 *
		 * In which case, move the window to the selected monitor,
		 * first.
		 */
		if (m != NULL && m != fw->m) {
			rectangle ms;

			ms.x = m->si->x;
			ms.y = m->si->y;
			ms.width = m->si->w;
			ms.height = m->si->h;

			/* then move to screen */
			fvwmrect_move_into_rectangle(&r, &ms);
			update_fvwm_monitor(fw);
		}

		s.x = page.x - m->virtual_scr.Vx;
		s.y = page.y - m->virtual_scr.Vy;
		s.width = monitor_get_all_widths();
		s.height = monitor_get_all_heights();
		fvwmrect_move_into_rectangle(&r, &s);

		final.x = r.x;
		final.y = r.y;
	}
	else if (mode == MOVE_SCREEN)
	{
		rectangle r;
		rectangle s;
		rectangle t;
		struct monitor	*m;
		char		*token;

		token = PeekToken(action, &action);
		m = monitor_resolve_name(token);
		if (m == NULL)
		{
			m = monitor_get_current();
		}

		s.x = m->si->x;
		s.y = m->si->y;
		s.width = m->si->w;
		s.height = m->si->h;

		do_animate = False;
		page.x = m->virtual_scr.Vx;
		page.y = m->virtual_scr.Vy;
		r.x = p.x;
		r.y = p.y;
		r.width = sz.width;
		r.height = sz.height;
		t.x = page.x - m->virtual_scr.Vx;
		t.y = page.y - m->virtual_scr.Vy;
		t.width = monitor_get_all_widths();
		t.height = monitor_get_all_heights();
		/* move to page first */
		fvwmrect_move_into_rectangle(&r, &t);
		/* then move to screen */
		fvwmrect_move_into_rectangle(&r, &s);
		final.x = r.x;
		final.y = r.y;

		fw->Desk = m->virtual_scr.CurrentDesk;
		fw->m = m;
	}
	else
	{
		final.x = p.x;
		final.y = p.y;
		n = GetMoveArguments(
			fw, &action, sz, &final, &fWarp, &fPointer, True);

		if (n != 2 || fPointer)
		{
			InteractiveMove(&w, exc, &final, fPointer);
		}
		else if (IS_ICONIFIED(fw))
		{
			SET_ICON_MOVED(fw, 1);
		}
	}

	if (w == FW_W_FRAME(fw))
	{
		d.x = final.x - fw->g.frame.x;
		d.y = final.y - fw->g.frame.y;
		if (do_animate)
		{
			position start = { -1, -1 };

			AnimatedMoveFvwmWindow(
				fw, w, start, final, fWarp, -1, NULL);
		}
		frame_setup_window(
			fw, final.x, final.y, fw->g.frame.width,
			fw->g.frame.height, True);
		if (fWarp & !do_animate)
		{
			char *cmd = "WarpToWindow 50 50";
			execute_function_override_window(
				NULL, exc, cmd, NULL, 0, fw);
		}
		if (IS_MAXIMIZED(fw))
		{
			fw->g.max.x += d.x;
			fw->g.max.y += d.y;
		}
		else
		{
			fw->g.normal.x += d.x;
			fw->g.normal.y += d.y;
		}
		update_absolute_geometry(fw);
		maximize_adjust_offset(fw);
		XFlush(dpy);
	}
	else /* icon window */
	{
		__move_icon(fw, final, p, do_animate, fWarp);
		XFlush(dpy);
	}
	focus_grab_buttons_on_layer(fw->layer);

	return;
}

void CMD_Move(F_CMD_ARGS)
{
	__move_window(F_PASS_ARGS, False, MOVE_NORMAL);
	update_fvwm_monitor(exc->w.fw);
}

void CMD_AnimatedMove(F_CMD_ARGS)
{
	__move_window(F_PASS_ARGS, True, MOVE_NORMAL);
	update_fvwm_monitor(exc->w.fw);
}

void CMD_MoveToPage(F_CMD_ARGS)
{
	__move_window(F_PASS_ARGS, False, MOVE_PAGE);
	update_fvwm_monitor(exc->w.fw);
}

void CMD_MoveToScreen(F_CMD_ARGS)
{
	__move_window(F_PASS_ARGS, False, MOVE_SCREEN);
	update_fvwm_monitor(exc->w.fw);
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

/* These functions do the SnapAttraction stuff. It takes x and y coordinates
 * (*px and *py) and returns the snapped values. */
#define NOSNAP -99999
#define MAXSCORE 99999999
static void DoSnapGrid(FvwmWindow *fw, rectangle self, position *new)
{
	int grid_x = self.x / fw->snap_grid_x * fw->snap_grid_x;
	int grid_y = self.y / fw->snap_grid_y * fw->snap_grid_y;

	if (fw->snap_grid_x > 1 && self.x != grid_x)
		new->x = (self.x - grid_x <= fw->snap_grid_x / 2) ? grid_x :
			grid_x + fw->snap_grid_x;

	if (fw->snap_grid_y > 1 && self.y != grid_y)
		new->y = (self.y - grid_y <= fw->snap_grid_y / 2) ? grid_y :
			grid_y + fw->snap_grid_y;
}
static void DoSnapMonitor(
	FvwmWindow *fw, rectangle self, position *new,
	int snapd, bool resist, bool all_edges)
{
	position mon;
	position mon_br;
	int tmp;
	position br = { self.x + self.width, self.y + self.height };
	struct
	{
		int x;
		int y;
	} score = { MAXSCORE, MAXSCORE };
	struct monitor *m;

	TAILQ_FOREACH(m, &monitor_q, entry)
	{
		mon.x = m->si->x;
		mon.y = m->si->y;
		mon_br.x = mon.x + m->si->w;
		mon_br.y = mon.y + m->si->h;

		/* vertically */
		if (!(br.x < mon.x || self.x > mon_br.x) &&
		    (all_edges || m->edge.bottom == MONITOR_OUTSIDE_EDGE))
		{
			tmp = resist ? mon_br.y : mon_br.y - snapd;
			if (br.y <= mon_br.y + snapd && br.y >= tmp)
				update_pos(&score.y, &(new->y), self.y,
					mon_br.y - self.height);
		}
		if (!(br.x < mon.x || self.x > mon_br.x) &&
		    (all_edges || m->edge.top == MONITOR_OUTSIDE_EDGE))
		{
			tmp = resist ? mon.y : mon.y + snapd;
			if (self.y <= tmp && self.y >= mon.y - snapd)
				update_pos(&score.y, &(new->y),
					self.y, mon.y);
		}

		/* horizontally */
		if (!(br.y < mon.y || self.y > mon_br.y) &&
		    (all_edges || m->edge.right == MONITOR_OUTSIDE_EDGE))
		{
			tmp = resist ? mon_br.x : mon_br.x - snapd;
			if (br.x <= mon_br.x + snapd && br.x >= tmp)
				update_pos(&score.x, &(new->x), self.x,
					mon_br.x - self.width);
		}
		if (!(br.y < mon.y || self.y > mon_br.y) &&
		    (all_edges || m->edge.left == MONITOR_OUTSIDE_EDGE))
		{
			tmp = resist ? mon.x : mon.x + snapd;
			if (self.x <= tmp && self.x >= mon.x - snapd)
				update_pos(&score.x, &(new->x),
					self.x, mon.x);
		}
	}
}
static void DoSnapWindow(FvwmWindow *fw, rectangle self, position *new)
{
	struct
	{
		int x;
		int y;
	} score = { MAXSCORE, MAXSCORE };
	FvwmWindow *tmp;
	int maskout = (SNAP_SCREEN | SNAP_SCREEN_WINDOWS |
			SNAP_SCREEN_ICONS | SNAP_SCREEN_ALL);
	size_rect scr;

	scr.width = monitor_get_all_widths();
	scr.height = monitor_get_all_heights();

	for (tmp = Scr.FvwmRoot.next; tmp; tmp = tmp->next)
	{
		rectangle other;

		if (fw->Desk != tmp->Desk || fw == tmp)
				continue;

		/* check snapping type */
		switch (fw->snap_attraction.mode & ~(maskout))
		{
		case SNAP_WINDOWS:  /* we only snap windows */
			if (IS_ICONIFIED(tmp) || IS_ICONIFIED(fw))
				continue;
			break;
		case SNAP_ICONS:  /* we only snap icons */
			if (!IS_ICONIFIED(tmp) || !IS_ICONIFIED(fw))
				continue;
		case SNAP_SAME:  /* we don't snap unequal */
			if (IS_ICONIFIED(tmp) != IS_ICONIFIED(fw))
				continue;
			break;
		default:  /* All */
			/* NOOP */
			break;
		}

		/* get other window dimensions */
		get_visible_window_or_icon_geometry(tmp, &other);
		if (other.x >= scr.width ||
		    other.x + other.width <= 0 ||
		    other.y >= scr.height ||
		    other.y + other.height <= 0)
		{
			/* do not snap to windows that are not currently
			 * visible */
			continue;
		}

		/* snap horizontally */
		if (other.y + other.height > self.y &&
			other.y < self.y + self.height)
		{
			if (self.x + self.width >= other.x -
			    fw->snap_attraction.proximity &&
			    self.x + self.width <= other.x +
			    fw->snap_attraction.proximity)
			{
				update_pos(&score.x, &(new->x), self.x,
					other.x - self.width);
			}
			if (self.x <= other.x + other.width +
			    fw->snap_attraction.proximity &&
			    self.x >= other.x + other.width -
			    fw->snap_attraction.proximity)
			{
				update_pos(&score.x, &(new->x), self.x,
					other.x + other.width);
			}
		}

		/* snap vertically */
		if (other.x + other.width > self.x &&
			other.x < self.x + self.width)
		{
			if (self.y + self.height >= other.y -
			    fw->snap_attraction.proximity &&
			    self.y + self.height <= other.y +
			    fw->snap_attraction.proximity)
			{
				update_pos(&score.y, &(new->y), self.y,
					other.y - self.height);
			}
			if (self.y <= other.y + other.height +
			    fw->snap_attraction.proximity &&
			    self.y >= other.y + other.height -
			    fw->snap_attraction.proximity)
			{
				update_pos(&score.y, &(new->y), self.y,
					other.y + other.height);
			}
		}
	}

	/* Ignore snapping to window sides on monitor edges */
	struct monitor *m;

	TAILQ_FOREACH(m, &monitor_q, entry)
	{
		if ((new->x == m->si->x || new->x == m->si->x + m->si->w) &&
			(self.y >= m->si->y &&
			self.y + self.height <= m->si->y + m->si->h))
		{
			new->x = NOSNAP;
		}
		if ((new->y == m->si->y || new->y == m->si->y + m->si->h) &&
			(self.x >= m->si->y &&
			self.x + self.width <= m->si->x + m->si->w))
		{
			new->y = NOSNAP;
		}
	}

}
static void DoSnapAttract(FvwmWindow *fw, size_rect sz, position *p)
{
	position new = { NOSNAP, NOSNAP };
	position win = { NOSNAP, NOSNAP };
	position mon = { NOSNAP, NOSNAP };
	rectangle self, g;

	self.x = p->x;
	self.y = p->y;
	self.width = sz.width;
	self.height = sz.height;
	if (get_visible_icon_title_geometry(fw, &g))
		self.height += g.height;

	int icon_mask = IS_ICONIFIED(fw) ? SNAP_SCREEN_ICONS :
		SNAP_SCREEN_WINDOWS;
	bool snap_mon = (fw->snap_attraction.mode & SNAP_SCREEN_ALL ||
		fw->snap_attraction.mode & (SNAP_SCREEN & icon_mask)) ?
		true : false;
	bool snap_win = (fw->snap_attraction.mode &
		(SNAP_ICONS | SNAP_WINDOWS | SNAP_SAME)) ? true : false;
	bool snap_grid = (fw->snap_grid_x > 1 || fw->snap_grid_y > 1) ? true :
		false;

	/*
	 * Checks both monitor edges and windows. Snaps to closer of the two.
         * If no monitor edge or window are found, snap to SnapGrid.
	 */
	if (fw->snap_attraction.proximity > 0)
	{
		if (snap_mon)
			DoSnapMonitor(fw, self, &mon,
				fw->snap_attraction.proximity, false, true);

		if (snap_win)
			DoSnapWindow(fw, self, &win);

		if (mon.x != NOSNAP || win.x != NOSNAP)
			new.x = (abs(self.x - mon.x) < abs(self.x - win.x)) ?
				mon.x : win.x;

		if (mon.y != NOSNAP || win.y != NOSNAP)
			new.y = (abs(self.y - mon.y) < abs(self.y - win.y)) ?
				mon.y : win.y;

		if ((new.x == NOSNAP || new.y == NOSNAP) && snap_grid)
		{
			win.x = NOSNAP;
			win.y = NOSNAP;
			DoSnapGrid(fw, self, &win);
			if (new.x == NOSNAP && win.x != NOSNAP)
				new.x = win.x;
			if (new.y == NOSNAP && win.y != NOSNAP)
				new.y = win.y;
		}
	}

	if (new.x != NOSNAP)
		p->x = new.x;
	if (new.y != NOSNAP)
		p->y = new.y;

	/* Resist moving windows beyond outside edges of the monitor */
	if (fw->edge_resistance_move > 0)
	{
		new.x = NOSNAP;
		new.y = NOSNAP;
		DoSnapMonitor(fw, self, &new,
			fw->edge_resistance_move, true, false);
		if (new.x != NOSNAP)
			p->x = new.x;
		if (new.y != NOSNAP)
			p->y = new.y;
	}

	/* Resist moving windows beyond all edges of the monitor. */
	if (fw->edge_resistance_screen_move > 0)
	{
		new.x = NOSNAP;
		new.y = NOSNAP;
		DoSnapMonitor(fw, self, &new,
			fw->edge_resistance_screen_move, true, true);
		if (new.x != NOSNAP)
			p->x = new.x;
		if (new.y != NOSNAP)
			p->y = new.y;
	}

	return;
}
#undef NOSNAP
#undef MAXSCORE

extern Window bad_window;

static void warp_pointer(position new)
{
	position pointer;
	Bool b;

	MyXGrabServer(dpy);
	b = FQueryPointer(
		dpy, Scr.Root, &JunkRoot, &JunkChild, &pointer.x, &pointer.y,
		&JunkX, &JunkY, &JunkMask);
	if (b == False)
	{
		goto out;
	}
	/*!!!handle failure*/
	FWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, new.x, new.y);
	XFlush(dpy);
  out:
	MyXUngrabServer(dpy);

	return;
}

enum
{
	MLI_CLEAR = 0,
	MLI_IS_FINISHED = 0x01,
	MLI_IS_ABORTED = 0x02,
	MLI_DO_RESIZE_TOO = 0x04,
};

typedef struct
{
	/*
	 * Parts that change during loop execution.
	 */
	FvwmWindow *fw;
	struct monitor *m;
	rectangle scr_g;
	position ppos;
	position wpos;
	position wpos_raw;
	position scr_center;
	position cn;
	unsigned int button_mask;
	Bool is_ppos_set;
	Bool sent_cn;
	Bool was_snapped;
	/* if Alt is initially pressed don't enable "Alt" mode until the key is
	 * released once. */
	Bool was_alt_key_not_pressed;
	/* Must not set placed by button if the event is a modified KeyEvent */
	Bool is_alt_mode_enabled;

	/*
	 * Parts that do not change.
	 */
	position porig;
	position virtual_scr;
	size_rect w_size;
	unsigned int draw_parts;
	Bool do_move_opaque;
} mli_state_t;

static void mli_update_alt_mode(mli_state_t *ls, unsigned int mod_state)
{
	if (mod_state & Mod1Mask)
	{
		ls->is_alt_mode_enabled = ls->was_alt_key_not_pressed;
	}
	else
	{
		ls->was_alt_key_not_pressed = True;
		ls->is_alt_mode_enabled = False;
	}

	return;
}

static unsigned int move_loop_inner(mli_state_t *ls)
{
	position new_ppos;
	unsigned int ret;
	Bool do_handle_movement;
	Bool do_allow_snap;
	XEvent e;
	unsigned int mods_state;

	ret = MLI_CLEAR;
	e.type = None;
	if (update_fvwm_monitor(ls->fw))
	{
		/*!!!recalculate scr_g*/
	}
	/* Keep the pointer away from the screen edges. */
	do
	{
		XEvent le;
		rectangle last_pos;
		bool b;

		fev_get_last_event(&le);
		b = fev_get_evpos(&le, &last_pos.x, &last_pos.y);
		if (!b)
		{
			break;
		}
		if (
			last_pos.x < ls->scr_g.width / 4 ||
			last_pos.x > 3 * ls->scr_g.width / 4 ||
			last_pos.y < ls->scr_g.height / 4 ||
			last_pos.y > 3 * ls->scr_g.height / 4)
		{
			/* Warp the pointer back to the center of screen. */
			warp_pointer(ls->scr_center);
			ls->ppos = ls->scr_center;
		}
	} while (0);
	/* block until an event arrives */
	FMaskEvent(
		dpy, ButtonPressMask | ButtonReleaseMask | KeyPressMask |
		PointerMotionMask | ButtonMotionMask | ExposureMask, &e);

	/* discard extra events before a logical release */
	if (
		e.type == MotionNotify ||
		e.type == EnterNotify || e.type == LeaveNotify)
	{
		while (
			FPending(dpy) > 0 && FCheckMaskEvent(
				dpy, ButtonMotionMask |
				PointerMotionMask | ButtonPressMask |
				ButtonRelease | KeyPressMask, &e))
		{
			if (
				e.type == ButtonPress ||
				e.type == ButtonRelease ||
				e.type == KeyPress)
			{
				break;
			}
		}
	}
	do_handle_movement = False;
	do_allow_snap = False;
	switch (e.type)
	{
	case KeyPress:
		mli_update_alt_mode(ls, e.xkey.state);
		Keyboard_shortcuts(&e, ls->fw, NULL, NULL, ButtonRelease);
		/* modifiers have already been handled by Keyboard_shortcuts().
		 */
		mods_state = 0;
		switch (e.type)
		{
		case ButtonRelease:
			new_ppos.x = e.xbutton.x_root;
			new_ppos.y = e.xbutton.y_root;
			do_handle_movement = True;
			ret |= MLI_IS_FINISHED;
			break;
		case MotionNotify:
			new_ppos.x = e.xmotion.x_root;
			new_ppos.y = e.xmotion.y_root;
			do_handle_movement = True;
			break;
		case KeyPress:
			if (XLookupKeysym(&(e.xkey), 0) == XK_Escape)
			{
				ret |= MLI_IS_ABORTED;
				ret |= MLI_IS_FINISHED;
			}
			break;
		default:
			/* bad event type, ignore it */
			return ret;
		}
		break;
	case ButtonPress:
		mods_state = e.xbutton.state;
		if (e.xbutton.button <= NUMBER_OF_MOUSE_BUTTONS &&
		    ((Button1Mask << (e.xbutton.button - 1)) &
		     ls->button_mask))
		{
			/* No new button was pressed, just a delayed event */
			break;
		}
		if (!IS_MAPPED(ls->fw) &&
		    (e.xbutton.button == 2 ||
		     (e.xbutton.button == 1 &&
		      (e.xbutton.state & ShiftMask))))
		{
			ret |= MLI_DO_RESIZE_TOO;
			/* Fallthrough to button-release */
		}
		else if (!ls->button_mask && e.xbutton.button <=
			 NUMBER_OF_EXTENDED_MOUSE_BUTTONS &&
			 e.xbutton.button > 0 &&
			 (move_interactive_finish_button_mask &
			  (1<<(e.xbutton.button-1))))
		{
			break;
		}
		else if (ls->button_mask &&  e.xbutton.button <=
			 NUMBER_OF_EXTENDED_MOUSE_BUTTONS &&
			 e.xbutton.button > 0 &&
			 (move_drag_finish_button_mask &
			  (1<<(e.xbutton.button-1))))
		{
			/* Fallthrough to button-release */
		}
		else
		{
			/* Abort the move if
			 *  - the move started with a pressed button another
			 *    button was pressed during the operation,
			 *  - any button not in the move_finish_button_mask is
			 *    pressed.
			 */
			ret |= MLI_IS_ABORTED;
			ret |= MLI_IS_FINISHED;
			break;
		}
	case ButtonRelease:
	{
		mods_state = e.xbutton.state;
		ls->fw->placed_by_button = e.xbutton.button;
		new_ppos.x = e.xbutton.x_root;
		new_ppos.y = e.xbutton.y_root;
		do_handle_movement = True;
		ret |= MLI_IS_FINISHED;
		break;
	}
	case MotionNotify:
	{
		mods_state = e.xmotion.state;
		mli_update_alt_mode(ls, e.xmotion.state);
		if (e.xmotion.same_screen == False)
		{
			return ret;
		}
		new_ppos.x = e.xmotion.x_root;
		new_ppos.y = e.xmotion.y_root;
		do_handle_movement = True;
		do_allow_snap = True;
		break;
	}
	case Expose:
		/* must undraw the rubber band in case the event causes some
		 * drawing */
		switch_move_resize_grid(False);
		dispatch_event(&e);
		if (!ls->do_move_opaque)
		{
			rectangle r = {
				ls->wpos.x, ls->wpos.y,
				ls->w_size.width - 1, ls->w_size.height - 1
			};

			draw_move_resize_grid(r);
		}
		break;

	default:
		/* cannot happen */
		break;
	} /* switch */
	if ((ret & MLI_IS_ABORTED) || (ret & MLI_IS_FINISHED))
	{
		switch_move_resize_grid(False);
	}
	if (do_handle_movement && !ls->is_ppos_set)
	{
		ls->ppos = new_ppos;
		ls->is_ppos_set = True;
		return ret;
	}
	if (do_handle_movement)
	{
		Bool may_snap;
		position dist;

		dist.x = new_ppos.x - ls->ppos.x;
		dist.y = new_ppos.y - ls->ppos.y;
		if (mods_state & ShiftMask)
		{
			dist.x *= 8;
			dist.y *= 8;
		}
		else if (mods_state & ControlMask)
		{
			dist.x /= 2;
			dist.y /= 2;
		}
		ls->wpos_raw.x += dist.x;
		ls->wpos_raw.y += dist.y;
		ls->ppos = new_ppos;

fprintf(stderr, "!!!new: %d %d, old: %d %d, dist %d %d\n", new_ppos.x, new_ppos.y, ls->ppos.x, ls->ppos.y, dist.x, dist.y);
		may_snap = do_allow_snap || ls->was_snapped;
		if (!may_snap)
		{
			may_snap =
				ls->wpos_raw.x != ls->porig.x ||
				ls->wpos_raw.y != ls->porig.y ||
				ls->virtual_scr.x != ls->m->virtual_scr.Vx ||
				ls->virtual_scr.y != ls->m->virtual_scr.Vy;
		}

		/* only snap if the window actually moved! */
		ls->wpos = ls->wpos_raw;
		if (!ls->is_alt_mode_enabled && (may_snap || 1))
		{
			DoSnapAttract(ls->fw, ls->w_size, &ls->wpos);
			ls->was_snapped = True;
		}

		if (ls->do_move_opaque)
		{
			if (IS_ICONIFIED(ls->fw))
			{
				set_icon_position(
					ls->fw, ls->wpos.x, ls->wpos.y);
				move_icon_to_position(ls->fw);
				broadcast_icon_geometry(ls->fw, False);
			}
			else
			{
				XMoveWindow(
					dpy, FW_W_FRAME(ls->fw),
					ls->wpos.x, ls->wpos.y);
			}
		}
		else
		{
			rectangle r = {
				ls->wpos.x, ls->wpos.y,
				ls->w_size.width - 1, ls->w_size.height - 1
			};

			draw_move_resize_grid(r);
		}
		DisplayPosition(ls->fw, &e, ls->wpos, False);

		/* dv (13-Jan-2014): Without this XFlush the modules
		 * (and probably other windows too) sometimes get their
		 * expose only after the next motion step.  */
		XFlush(dpy);
	}

	if (ls->do_move_opaque && !IS_ICONIFIED(ls->fw))
	{
		if (!IS_SHADED(ls->fw) && !Scr.bo.do_disable_configure_notify)
		{
			/* send configure notify event for windows that care
			 * about their location; don't send anything if
			 * position didn't change */
			if (
				!ls->sent_cn || ls->cn.x != ls->wpos.x ||
				ls->cn.y != ls->wpos.y)
			{
				ls->cn = ls->wpos;
				ls->sent_cn = True;
				SendConfigureNotify(
					ls->fw, ls->wpos.x, ls->wpos.y,
					ls->w_size.width, ls->w_size.height,
					0, False);
				if (DEBUG_CONFIGURENOTIFY)
				{
					fvwm_debug(
						__func__,
						"Sent ConfigureNotify"
						" (w %d, h %d)",
						ls->w_size.width,
						ls->w_size.height);
				}
			}
		}
		if (do_handle_movement == True)
		{
			/* only do this with opaque moves, (i.e. the
			 * server is not grabbed) */
			if (ls->draw_parts != PART_NONE)
			{
				border_draw_decorations(
					ls->fw, ls->draw_parts,
					((ls->fw == get_focus_window())) ?
					True : False,
					True, CLEAR_ALL, NULL, NULL);
			}
			if (IS_TEAR_OFF_MENU(ls->fw))
			{
				menu_redraw_transparent_tear_off_menu(
					ls->fw, False);
			}
			rectangle t = ls->fw->g.frame;
			ls->fw->g.frame.x = ls->wpos.x;
			ls->fw->g.frame.y = ls->wpos.y;
			BroadcastConfig(M_CONFIGURE_WINDOW, ls->fw);
			ls->fw->g.frame = t;
			FlushAllMessageQueues();
		}
	}

	return ret;
}

/*
 *
 * Move the rubberband around, return with the new window location
 *
 * Returns True if the window has to be resized after the move.
 *
 */
Bool move_loop(
	const exec_context_t *exc, position _offset_w_to_p, size_rect _sz,
	position *_pFinal, Bool _do_move_opaque)
{
	unsigned int mli_ret = MLI_CLEAR;
	Window move_w;
	FvwmWindow *fw = exc->w.fw;
	mli_state_t ls;

	bad_window = None;
	memset(&ls, 0, sizeof(ls));
	if (fw->m == NULL)
	{
		update_fvwm_monitor(fw);
	}
	ls.m = fw->m;
	ls.fw = fw,
	ls.w_size = _sz;
	ls.virtual_scr.x = ls.m->virtual_scr.Vx;
	ls.virtual_scr.y = ls.m->virtual_scr.Vy;
	ls.do_move_opaque = _do_move_opaque;
	/*!!!_offset_w_to_p is unused*/
	ls.button_mask &= DEFAULT_ALL_BUTTONS_MASK;
	/* Fetch screen layout for use with elastic paging.*/
	FScreenGetScrRect(
		NULL, FSCREEN_CURRENT,
		&ls.scr_g.x, &ls.scr_g.y, &ls.scr_g.width, &ls.scr_g.height);
	ls.scr_center.x = ls.scr_g.x + ls.scr_g.width / 2;
	ls.scr_center.y = ls.scr_g.y + ls.scr_g.height / 2;

	if (!GrabEm(CRS_HIDDEN, GRAB_NORMAL))
	{
		XBell(dpy, 0);
		return False;
	}
	/* Unset the placed by button mask. If the move is canceled this will
	 * remain as zero. */
	fw->placed_by_button = 0;
	if (IS_ICONIFIED(fw))
	{
		/* window iconified */
		if (FW_W_ICON_PIXMAP(fw) != None)
		{
			move_w = FW_W_ICON_PIXMAP(fw);
		}
		else if (FW_W_ICON_TITLE(fw) != None)
		{
			move_w = FW_W_ICON_TITLE(fw);
		}
		else
		{
			mli_ret = MLI_CLEAR;
			goto out;
		}
		{
			rectangle g;

			get_visible_icon_geometry(fw, &g);
			ls.porig.x = g.x;
			ls.porig.y = g.y;
		}
	}
	else if (IS_MAPPED(fw))
	{
		/* window is mapped */
		move_w = FW_W_FRAME(fw);
		if (ls.do_move_opaque)
		{
			ls.draw_parts =
				border_get_transparent_decorations_part(fw);
		}
		if (!XGetGeometry(
			dpy, move_w, &JunkRoot, &ls.porig.x, &ls.porig.y,
			(unsigned int*)&JunkWidth, (unsigned int*)&JunkHeight,
			(unsigned int*)&JunkBW, (unsigned int*)&JunkDepth))
		{
			/* Don't care, just use the default position (0 0). */
		}
	}
	else
	{
		/* initial placement: window is not mapped and not iconified */
		ls.do_move_opaque = False;
		move_w = FW_W_FRAME(fw);
		{
			/* draw initial outline */
			rectangle r = {
				ls.porig.x, ls.porig.y,
				ls.w_size.width - 1, ls.w_size.height - 1
			};

			draw_move_resize_grid(r);
		}
	}
	ls.wpos_raw = ls.porig;
	ls.wpos = ls.porig;
	/* prevent flicker when paging */
	SET_WINDOW_BEING_MOVED_OPAQUE(fw, ls.do_move_opaque);
	DisplayPosition(fw, exc->x.elast, ls.wpos, True);

	warp_pointer(ls.scr_center);
	do
	{
		mli_ret = move_loop_inner(&ls);
	} while (!(mli_ret & MLI_IS_FINISHED) && bad_window != FW_W(fw));

	if (!Scr.gs.do_hide_position_window)
	{
		XUnmapWindow(dpy,Scr.SizeWindow.win);
	}
	if ((mli_ret & MLI_IS_ABORTED) || bad_window == FW_W(fw))
	{
		/* aborted */
		if (
			ls.virtual_scr.x != ls.m->virtual_scr.Vx ||
			ls.virtual_scr.y != ls.m->virtual_scr.Vy)
		{
			MoveViewport(
				ls.m, ls.virtual_scr.x, ls.virtual_scr.y,
				False);
		}
		if ((mli_ret & MLI_IS_ABORTED) && ls.do_move_opaque)
		{
			XMoveWindow(dpy, move_w, ls.porig.x, ls.porig.y);
			if (ls.draw_parts != PART_NONE)
			{
				border_draw_decorations(
					fw, ls.draw_parts,
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
	else
	{
		/* finished */
		_pFinal->x = ls.wpos.x;
		_pFinal->y = ls.wpos.y;
		if (IS_ICONIFIED(fw))
		{
			SET_ICON_MOVED(fw, 1);
		}
	}
	BroadcastConfig(M_CONFIGURE_WINDOW, fw);
	FlushAllMessageQueues();
  out:
	UngrabEm(GRAB_NORMAL);
	if (!(mli_ret & MLI_DO_RESIZE_TOO))
	{
		/* Don't wait for buttons to come up when user is placing a new
		 * window and wants to resize it. */
		WaitForButtonsUp(True);
	}
	SET_WINDOW_BEING_MOVED_OPAQUE(fw, 0);
	bad_window = None;

	return !!(mli_ret & MLI_DO_RESIZE_TOO);
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

static void set_geom_win_visible_val(char *token, bool val)
{
	Scr.gs.do_hide_position_window = !val;
	Scr.gs.do_hide_resize_window = !val;

	if (token == NULL)
	{
		return;
	}
	else if (StrEquals(token, "never"))
	{
		Scr.gs.do_hide_position_window = val;
		Scr.gs.do_hide_resize_window = val;
	}
	else if (StrEquals(token, "move"))
	{
		Scr.gs.do_hide_resize_window = val;
	}
	else if (StrEquals(token, "resize"))
	{
		Scr.gs.do_hide_position_window = val;
	}
}

static bool set_geom_win_position_val(char *s, int *coord, bool *neg, bool *rel)
{
	int val, n;

	if (sscanf(s, "-%d%n", &val, &n) >= 1)
	{
		*coord = val;
		*neg = true;
	}
	else if (
		sscanf(s, "+%d%n", &val, &n) >= 1 ||
		sscanf(s, "%d%n", &val, &n) >= 1)
	{
		*coord = val;
		*neg = false;
	}
	else
	{
		/* syntax error */
		return false;
	}
	s += n;
	*rel = (*s == 'p') ? false : true;

	return true;
}

void CMD_GeometryWindow(F_CMD_ARGS)
{
	int val;
	char *token;

	while ((token = PeekToken(action, &action)) != NULL)
	{
		if (StrEquals(token, "hide"))
		{
			token = PeekToken(action, &action);
			set_geom_win_visible_val(token, false);
		}
		if (StrEquals(token, "show"))
		{
			token = PeekToken(action, &action);
			set_geom_win_visible_val(token, true);
		}
		if (StrEquals(token, "colorset"))
		{
			if (GetIntegerArguments(action, &action, &val, 1) != 1)
			{
				val = -1;
			}
			Scr.SizeWindow.cset = val;
		}
		if (StrEquals(token, "position"))
		{
			Scr.SizeWindow.is_configured = false;
			/* x-coordinate */
			token = PeekToken(action, &action);
			if (!token)
			{
				continue;
			}
			if (
				!set_geom_win_position_val(
					token, &Scr.SizeWindow.x,
					&Scr.SizeWindow.xneg,
					&Scr.SizeWindow.xrel))
			{
				continue;
			}
			/* y-coordinate */
			token = PeekToken(action, &action);
			if (!token)
			{
				continue;
			}
			if (
				!set_geom_win_position_val(
					token, &Scr.SizeWindow.y,
					&Scr.SizeWindow.yneg,
					&Scr.SizeWindow.yrel))
			{
				continue;
			}
			Scr.SizeWindow.is_configured = true;
		}
		if (StrEquals(token, "screen"))
		{
			token = PeekToken(action, &action);
			if (token != NULL)
			{
				Scr.SizeWindow.m = monitor_resolve_name(token);
			}
		}
	}
}

void CMD_HideGeometryWindow(F_CMD_ARGS)
{
	char *cmd;

	fvwm_debug(__func__, "HideGeometryWindow is deprecated.  "
	    "Converting to use: GeometryWindow hide %s", action);

	xasprintf(&cmd, "GeometryWindow hide %s", action);
	execute_function_override_window(NULL, NULL, cmd, NULL, 0, NULL);
	free(cmd);
}

void CMD_SnapAttraction(F_CMD_ARGS)
{
	char *cmd;
	size_t len;

	/* TA:  FIXME  xasprintf() */
	len = strlen(action);
	len += 99;
	cmd = fxmalloc(len);
	sprintf(cmd, "Style * SnapAttraction %s", action);
	fvwm_debug(__func__,
		   "The command SnapAttraction is obsolete. Please use the"
		   " following command instead:\n\n%s", cmd);
	execute_function(cond_rc, exc, cmd, pc, FUNC_DONT_EXPAND_COMMAND);
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
	cmd = fxmalloc(len);
	sprintf(cmd, "Style * SnapGrid %s", action);
	fvwm_debug(__func__,
		   "The command SnapGrid is obsolete. Please use the following"
		   " command instead:\n\n%s", cmd);
	execute_function(cond_rc, exc, cmd, pc, FUNC_DONT_EXPAND_COMMAND);
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

	/* free up XorPixmap if necessary */
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
		fvwm_debug(__func__, "Can't find pixmap %s", PixmapName);
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
	position off, position p)
{
	direction_t dir = DIR_NONE;
	position t;
	position d;

	if (p.x < 0 || off.x < 0 || p.y < 0 || off.y < 0)
	{
		return dir;
	}

	/* Rough quadrants per window.  3x3. */
	t.x = (off.x / 3) - 1;
	t.y = (off.y / 3) - 1;

	d.x = off.x - p.x;
	d.y = off.y - p.y;

	if (p.x < t.x)
	{
		/* Far left of window.  Quadrants of NW, W, SW. */
		if (p.y < t.y)
		{
			/* North-West direction. */
			dir = DIR_NW;
		}
		else if (d.y < t.y)
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
	else if (d.x < t.x)
	{
		/* Far right of window.  Quadrants NE, E, SE. */
		if (p.y < t.y)
		{
			/* North-East direction. */
			dir = DIR_NE;
		}
		else if (d.y < t.y)
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
		if (p.y < t.y)
		{
			/* North direction. */
			dir = DIR_N;
		}
		else if (d.y < t.y)
		{
			/* South direction. */
			dir = DIR_S;
		}
	}

	return dir;
}

static void __resize_get_dir_from_window(
	position *ret_motion, FvwmWindow *fw, Window context_w)
{
	if (context_w != Scr.Root && context_w != None)
	{
		if (context_w == FW_W_SIDE(fw, 0))   /* top */
		{
			ret_motion->y = 1;
		}
		else if (context_w == FW_W_SIDE(fw, 1))  /* right */
		{
			ret_motion->x = -1;
		}
		else if (context_w == FW_W_SIDE(fw, 2))  /* bottom */
		{
			ret_motion->y = -1;
		}
		else if (context_w == FW_W_SIDE(fw, 3))  /* left */
		{
			ret_motion->x = 1;
		}
		else if (context_w == FW_W_CORNER(fw, 0))  /* upper-left */
		{
			ret_motion->x = 1;
			ret_motion->y = 1;
		}
		else if (context_w == FW_W_CORNER(fw, 1))  /* upper-right */
		{
			ret_motion->x = -1;
			ret_motion->y = 1;
		}
		else if (context_w == FW_W_CORNER(fw, 2)) /* lower left */
		{
			ret_motion->x = 1;
			ret_motion->y = -1;
		}
		else if (context_w == FW_W_CORNER(fw, 3))  /* lower right */
		{
			ret_motion->x = -1;
			ret_motion->y = -1;
		}
	}

	return;
}

static void __resize_get_dir_proximity(
	position *ret_motion, FvwmWindow *fw, position off,
	position p, position *warp, Bool find_nearest_border)
{
	position t;
	direction_t dir;

	warp->x = warp->y = -1;

	if (p.x < 0 || off.x < 0 || p.y < 0 || off.y < 0)
	{
		return;
	}

	if (find_nearest_border == False)
	{
		t.x = 0;
		t.y = 0;

		t.x = max(fw->boundary_width, t.x);
		t.y = max(fw->boundary_width, t.y);

		if (p.x < t.x)
		{
			ret_motion->x = 1;
		}
		else if (off.x < t.x)
		{
			ret_motion->x = -1;
		}
		if (p.y < t.y)
		{
			ret_motion->y = 1;
		}
		else if (off.y < t.y)
		{
			ret_motion->y = -1;
		}

		return;
	}

	/* Get the direction from the quadrant the pointer is in. */
	dir = __resize_get_dir_from_resize_quadrant(off, p);

	switch (dir)
	{
	case DIR_NW:
		ret_motion->x = 1;
		ret_motion->y = 1;
		warp->x = 0;
		warp->y = 0;
		break;
	case DIR_SW:
		ret_motion->x = 1;
		ret_motion->y = -1;
		warp->x = 0;
		warp->y = (off.y - 1);
		break;
	case DIR_W:
		ret_motion->x = 1;
		warp->x = 0;
		warp->y = (off.y / 2);
		break;
	case DIR_NE:
		ret_motion->x = -1;
		ret_motion->y = 1;
		warp->x = (off.x - 1);
		warp->y = 0;
		break;
	case DIR_SE:
		ret_motion->x = -1;
		ret_motion->y = -1;
		warp->x = (off.x - 1);
		warp->y = (off.y - 1);
		break;
	case DIR_E:
		ret_motion->x = -1;
		warp->x = (off.x - 1);
		warp->y = (off.y / 2);
		break;
	case DIR_N:
		ret_motion->y = 1;
		warp->x = (off.x / 2);
		warp->y = 0;
		break;
	case DIR_S:
		ret_motion->y = -1;
		warp->x = (off.x / 2);
		warp->y = (off.y - 1);
		break;
	default:
		break;
	}

	return;
}

static void __resize_get_refpos(
	position *ret, position motion, size_rect sz, FvwmWindow *fw)
{
	if (motion.x > 0)
	{
		ret->x = 0;
	}
	else if (motion.x < 0)
	{
		ret->x = sz.width - 1;
	}
	else
	{
		ret->x = sz.width / 2;
	}
	if (motion.y > 0)
	{
		ret->y = 0;
	}
	else if (motion.y < 0)
	{
		ret->y = sz.height - 1;
	}
	else
	{
		ret->y = sz.height / 2;
	}

	return;
}

/* Procedure:
 *      __resize_step - move the rubberband around.  This is called for
 *                 each motion event when we are resizing
 *
 *  Inputs:
 *      root     - the corrdinates in the root window
 *      off      - offset of pointer from border (input/output)
 *      drag     - resize internal structure
 *      orig     - resize internal structure
 *      pmotion  - pointer to motion in resize_window
 *
 */
static void __resize_step(
	const exec_context_t *exc, position root, position *off,
	rectangle *drag, const rectangle *orig, position *pmotion,
	Bool do_resize_opaque, Bool is_direction_fixed)
{
	int action = 0;
	position p2;
	position dir;

	p2.x = root.x - off->x;
	root.x += off->x;
	if (is_direction_fixed == True && (pmotion->x != 0 || pmotion->y != 0))
	{
		dir.x = pmotion->x;
	}
	else if (p2.x <= orig->x ||
		 (pmotion->x == 1 && p2.x < orig->x + orig->width - 1))
	{
		dir.x = 1;
	}
	else if (p2.x >= orig->x + orig->width - 1 ||
		 (pmotion->x == -1 && p2.x > orig->x))
	{
		dir.x = -1;
	}
	else
	{
		dir.x = 0;
	}
	switch (dir.x)
	{
	case 1:
		if (pmotion->x != 1)
		{
			off->x = -off->x;
			root.x = p2.x;
			pmotion->x = 1;
		}
		drag->x = root.x;
		drag->width = orig->x + orig->width - root.x;
		action = 1;
		break;
	case -1:
		if (pmotion->x != -1)
		{
			off->x = -off->x;
			root.x = p2.x;
			pmotion->x = -1;
		}
		drag->x = orig->x;
		drag->width = 1 + root.x - drag->x;
		action = 1;
		break;
	default:
		break;
	}
	p2.y = root.y - off->y;
	root.y += off->y;
	if (is_direction_fixed == True && (pmotion->x != 0 || pmotion->y != 0))
	{
		dir.y = pmotion->y;
	}
	else if (p2.y <= orig->y ||
		 (pmotion->y == 1 && p2.y < orig->y + orig->height - 1))
	{
		dir.y = 1;
	}
	else if (p2.y >= orig->y + orig->height - 1 ||
		 (pmotion->y == -1 && p2.y > orig->y))
	{
		dir.y = -1;
	}
	else
	{
		dir.y = 0;
	}
	switch (dir.y)
	{
	case 1:
		if (pmotion->y != 1)
		{
			off->y = -off->y;
			root.y = p2.y;
			pmotion->y = 1;
		}
		drag->y = root.y;
		drag->height = orig->y + orig->height - root.y;
		action = 1;
		break;
	case -1:
		if (pmotion->y != -1)
		{
			off->y = -off->y;
			root.y = p2.y;
			pmotion->y = -1;
		}
		drag->y = orig->y;
		drag->height = 1 + root.y - drag->y;
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
			pmotion->x, pmotion->y, CS_ROUND_UP);
		if (pmotion->x == 1)
		{
			drag->x = orig->x + orig->width - drag->width;
		}
		if (pmotion->y == 1)
		{
			drag->y = orig->y + orig->height - drag->height;
		}
		if (!do_resize_opaque)
		{
			rectangle r = {
				drag->x, drag->y,
				drag->width - 1, drag->height - 1
			};

			draw_move_resize_grid(r);
		}
		else
		{
			frame_setup_window(
				exc->w.fw, drag->x, drag->y, drag->width,
				drag->height, False);
		}
	}
	{
		size_rect sz = { drag->width, drag->height };

		DisplaySize(exc->w.fw, exc->x.elast, sz, False, False);
	}

	return;
}

/* Starts a window resize operation */
static Bool __resize_window(F_CMD_ARGS)
{
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
	Bool was_alt_key_not_pressed = False;
	Bool is_alt_mode_enabled = False;
	position p;
	position p2;
	position delta;
	position stashed;
	position d;
	position motion = { 0, 0 };
	position edge_wrap;
	position ref;
	position off;
	position warp = { 0, 0 };
	Window ResizeWindow;
	struct monitor *mon = fw->m;
	const position v = { mon->virtual_scr.Vx, mon->virtual_scr.Vy };
	int n;
	unsigned int button_mask = 0;
	rectangle sdrag;
	rectangle sorig;
	rectangle *drag = &sdrag;
	const rectangle *orig = &sorig;
	const window_g g_backup = fw->g;
	int was_maximized;
	int i;
	size_borders b;
	frame_move_resize_args mr_args = NULL;
	long evmask;
	XEvent ev;
	direction_t dir;
	/* if Alt is initially pressed don't enable "Alt" mode until the key is
	 * released once. */

	d.x = mon->virtual_scr.EdgeScrollX ?
		mon->virtual_scr.EdgeScrollX : monitor_get_all_widths();
	d.y = mon->virtual_scr.EdgeScrollY ?
		mon->virtual_scr.EdgeScrollY : monitor_get_all_heights();

	bad_window = False;
	ResizeWindow = FW_W_FRAME(fw);
	if (fev_get_evpos_or_query(
		    dpy, Scr.Root, exc->x.etrigger, &p2.x, &p2.y) ==
	    False ||
	    XTranslateCoordinates(
		    dpy, Scr.Root, ResizeWindow, p2.x, p2.y, &p2.x, &p2.y,
		    &JunkChild) == False)
	{
		/* pointer is on a different screen - that's okay here */
		p2.x = 0;
		p2.y = 0;
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
	{
		position fp = { fw->g.frame.x, fw->g.frame.y };
		size_rect sz = { drag->width, drag->height };
		size_rect base = {
			fw->hints.base_width, fw->hints.base_height
		};
		size_rect inc = {
			fw->hints.width_inc, fw->hints.height_inc,
		};

		n = GetResizeArguments(
			fw,
			&action, fp,
			base, inc,
			&b, &sz,
			&dir, &is_direction_fixed, &do_warp_to_border,
			&automatic_border_direction,
			&detect_automatic_direction);
		drag->width = sz.width;
		drag->height = sz.height;
	}

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
				fw, NULL, &drag->width, &drag->height, motion.x,
				motion.y, 0);
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
				fw, NULL, &drag->width, &drag->height, motion.x,
				motion.y, 0);
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
	edge_wrap.x = Scr.flags.do_edge_wrap_x;
	edge_wrap.y = Scr.flags.do_edge_wrap_y;
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
	motion.y = 0;
	motion.x = 0;

	/* pop up a resize dimensions window */
	if (!Scr.gs.do_hide_resize_window)
	{
		resize_geometry_window();
		position_geometry_window(NULL);
		XMapRaised(dpy, Scr.SizeWindow.win);
	}
	{
		size_rect sz = { orig->width, orig->height };

		DisplaySize(fw, exc->x.elast, sz, True, True);
	}
	if (dir == DIR_NONE && detect_automatic_direction == True)
	{
		position toff = { orig->width, orig->height };

		dir = __resize_get_dir_from_resize_quadrant(toff, p2);
	}

	if (dir != DIR_NONE)
	{
		int grav;

		grav = gravity_dir_to_grav(dir);
		gravity_get_offsets(grav, &motion.x, &motion.y);
		motion.x = -motion.x;
		motion.y = -motion.y;
	}
	if (motion.x == 0 && motion.y == 0)
	{
		__resize_get_dir_from_window(&motion, fw, PressedW);
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
	/* don't warp if the resize was triggered by a press somewhere on the
	 * title bar */
	if (PressedW != Scr.Root && motion.x == 0 && motion.y == 0 &&
	    !called_from_title)
	{
		position toff = { orig->width, orig->height };

		__resize_get_dir_proximity(
			&motion, fw, toff, p2, &warp,
			automatic_border_direction);
		if (motion.x != 0 || motion.y != 0)
		{
			do_warp_to_border = True;
		}
	}
	if (!IS_SHADED(fw))
	{
		size_rect sz = { orig->width, orig->height };

		__resize_get_refpos(&ref, motion, sz, fw);
	}
	else
	{
		switch (SHADED_DIR(fw))
		{
		case DIR_N:
		case DIR_NW:
		case DIR_NE:
			if (motion.y == -1)
			{
				motion.y = 0;
			}
			break;
		case DIR_S:
		case DIR_SW:
		case DIR_SE:
			if (motion.y == 1)
			{
				motion.y = 0;
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
			if (motion.x == 1)
			{
				motion.x = 0;
			}
			break;
		case DIR_W:
		case DIR_NW:
		case DIR_SW:
			if (motion.x == -1)
			{
				motion.x = 0;
			}
			break;
		default:
			break;
		}
		{
			size_rect sz = {
				fw->g.frame.width, fw->g.frame.height
			};

			__resize_get_refpos(&ref, motion, sz, fw);
		}
	}
	off.x = 0;
	off.y = 0;
	if (do_warp_to_border == True)
	{
		position dd;

		dd.x = (motion.x == 0) ? p2.x : ref.x;
		dd.y = (motion.y == 0) ? p2.y : ref.y;

		/* Warp the pointer to the closest border automatically? */
		if (automatic_border_direction == True &&
			(warp.x >=0 && warp.y >=0) &&
			!IS_SHADED(fw))
		{
			dd.x = warp.x;
			dd.y = warp.y;
		}

		/* warp the pointer to the border */
		FWarpPointer(dpy, None, ResizeWindow, 0, 0, 1, 1, dd.x, dd.y);
		XFlush(dpy);
	}
	else if (motion.x != 0 || motion.y != 0)
	{
		/* keep the distance between pointer and border */
		off.x = (motion.x == 0) ? 0 : ref.x - p2.x;
		off.y = (motion.y == 0) ? 0 : ref.y - p2.y;
	}
	else
	{
		/* wait until the pointer hits a border before making a
		 * decision about the resize direction */
	}

	/* draw the rubber-band window */
	if (!do_resize_opaque)
	{
		rectangle r = {
			drag->x, drag->y,
			drag->width - 1, drag->height - 1
		};

		draw_move_resize_grid(r);
	}
	/* kick off resizing without requiring any motion if invoked with a key
	 * press */
	if (exc->x.elast->type == KeyPress)
	{
		position toff = { 0, 0 };

		if (FQueryPointer(
			    dpy, Scr.Root, &JunkRoot, &JunkChild, &stashed.x,
			    &stashed.y, &JunkX, &JunkY, &JunkMask) == False)
		{
			/* pointer is on a different screen */
			stashed.x = 0;
			stashed.y = 0;
		}
		__resize_step(
			exc, stashed, &toff, drag, orig,
			&motion, do_resize_opaque, True);
	}
	else
	{
		stashed.x = stashed.y = -1;
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
			int delay;

			delay = (is_alt_mode_enabled) ?
				0 : fw->edge_delay_ms_resize;
			rc = HandlePaging(
				&ev, d, &p, &delta, False, False, True, delay);
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
			if (ev.xkey.state & Mod1Mask)
			{
				is_alt_mode_enabled = was_alt_key_not_pressed;
			}
			else
			{
				was_alt_key_not_pressed = True;
				is_alt_mode_enabled = False;
			}
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
			if (ev.xmotion.state & Mod1Mask)
			{
				is_alt_mode_enabled = was_alt_key_not_pressed;
			}
			else
			{
				was_alt_key_not_pressed = True;
				is_alt_mode_enabled = False;
			}
			if (ev.xmotion.same_screen == False)
			{
				continue;
			}
			if (!fForceRedraw)
			{
				int delay;

				delay = (is_alt_mode_enabled) ?
					0 : fw->edge_delay_ms_resize;
				p.x = ev.xmotion.x_root;
				p.y = ev.xmotion.y_root;
				/* resize before paging request to prevent
				 * resize from lagging * mouse - mab */
				__resize_step(
					exc, p, &off, drag, orig,
					&motion, do_resize_opaque,
					is_direction_fixed);
				is_resized = True;
				/* need to move the viewport */
				HandlePaging(
					&ev, d, &p, &delta,
					False, False, False, delay);
			}
			/* redraw outline if we paged - mab */
			if (delta.x != 0 || delta.y != 0)
			{
				sorig.x -= delta.x;
				sorig.y -= delta.y;
				drag->x -= delta.x;
				drag->y -= delta.y;

				__resize_step(
					exc, p, &off, drag, orig,
					&motion, do_resize_opaque,
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
				rectangle r = {
					drag->x, drag->y,
					drag->width - 1, drag->height - 1
				};

				draw_move_resize_grid(r);
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
		XUnmapWindow(dpy, Scr.SizeWindow.win);
	}
	if (is_aborted || bad_window == FW_W(fw))
	{
		/* return pointer if aborted resize was invoked with key */
		if (stashed.x >= 0)
		{
			FWarpPointer(
				dpy, None, Scr.Root, 0, 0, 0, 0, stashed.x,
				stashed.y);
		}
		if (was_maximized)
		{
			/* since we aborted the resize, the window is still
			 * maximized */
			SET_MAXIMIZED(fw, 1);
		}
		if (do_resize_opaque)
		{
			position toff = { 0, 0 };
			rectangle g = sorig;
			position psorig = { sorig.x, sorig.y };

			motion.x = 1;
			motion.y = 1;
			__resize_step(
				exc, psorig, &toff, &g, orig,
				&motion, do_resize_opaque, True);
		}
		if (v.x != mon->virtual_scr.Vx || v.y != mon->virtual_scr.Vy)
		{
			MoveViewport(mon, v.x, v.y, False);
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
			motion.x, motion.y, CS_ROUND_UP);
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
	Scr.flags.do_edge_wrap_x = edge_wrap.x;
	Scr.flags.do_edge_wrap_y = edge_wrap.y;
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
	update_fvwm_monitor(fw);

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
	FvwmWindow *fw, position *p11, position *p12)
{
	position p21;
	position p22;
	position page;

	get_page_offset_check_visible(&page.x, &page.y, fw);
	p21.x = page.x;
	p22.x = page.x + monitor_get_all_widths();
	p21.y = page.y;
	p22.y = page.y + monitor_get_all_heights();

	/* make sure the x coordinate is on the same page as the reference
	 * window */
	if (p11->x >= p22.x)
	{
		while (p11->x >= p22.x)
		{
			p11->x -= monitor_get_all_widths();
			p12->x -= monitor_get_all_widths();
		}
	}
	else if (p12->x <= p21.x)
	{
		while (p12->x <= p21.x)
		{
			p11->x += monitor_get_all_widths();
			p12->x += monitor_get_all_widths();
		}
	}
	/* make sure the y coordinate is on the same page as the reference
	 * window */
	if (p11->y >= p22.y)
	{
		while (p11->y >= p22.y)
		{
			p11->y -= monitor_get_all_heights();
			p12->y -= monitor_get_all_heights();
		}
	}
	else if (p12->y <= p21.y)
	{
		while (p12->y <= p21.y)
		{
			p11->y += monitor_get_all_heights();
			p12->y += monitor_get_all_heights();
		}
	}

	return;
}


/*
 * Grows a window rectangle until its edges touch the closest window based
 * on snap type, layers = { min_layer, max_layer }, or the boundary rectangle.
 */
static void grow_to_closest_type(
	FvwmWindow *fw, rectangle *win_r, rectangle bound, int *layers,
	int type, bool consider_touching)
{
	FvwmWindow *twin;
	rectangle other;
	int maskout = (SNAP_SCREEN | SNAP_SCREEN_WINDOWS |
			SNAP_SCREEN_ICONS | SNAP_SCREEN_ALL);

	/* window coordinates for original window, other, and new */
	position pw1;
	position pw2;
	position po1;
	position po2;
	position new1;
	position new2;

	pw1.x = win_r->x;
	pw2.x = pw1.x + win_r->width;
	pw1.y = win_r->y;
	pw2.y = pw1.y + win_r->height;

	new1.x = bound.x;
	new2.x = new1.x + bound.width;
	new1.y = bound.y;
	new2.y = new1.y + bound.height;

	/* Use other windows to shrink boundary to get closest */
	for (twin = Scr.FvwmRoot.next; twin; twin = twin->next)
	{
		if (twin == fw || IS_TRANSIENT(fw) ||
			!IS_PARTIALLY_VISIBLE(twin) ||
			(twin->Desk != fw->Desk &&
			!is_window_sticky_across_desks(twin)))
		{
			continue;
		}
		switch (type & ~(maskout))
		{
		case SNAP_WINDOWS:  /* we only consider windows */
			if (IS_ICONIFIED(twin))
			{
				continue;
			}
			break;
		case SNAP_ICONS:  /* we only consider icons */
			if (!IS_ICONIFIED(twin))
			{
				continue;
			}
			break;
		case SNAP_SAME:  /* we don't consider unequal */
			if (IS_ICONIFIED(twin) != IS_ICONIFIED(fw))
			{
				continue;
			}
			break;
		default:
			break;
		}
		if ((layers[0] >= 0 && twin->layer < layers[0]) ||
			(layers[1] >= 0 && twin->layer > layers[1]))
		{
			continue;
		}
		if (!get_visible_window_or_icon_geometry(twin, &other))
		{
			continue;
		}
		po1.x = other.x;
		po2.x = po1.x + other.width;
		po1.y = other.y;
		po2.y = po1.y + other.height;

		if (is_window_sticky_across_pages(twin))
                {
                        move_sticky_window_to_same_page(fw, &po1, &po2);
                }

		/* Shrink left/right edges */
		if (po1.y < pw2.y && po2.y > pw1.y)
		{
			if (new1.x < po2.x && (pw1.x > po2.x ||
				(consider_touching && pw1.x == po2.x)))
			{
				new1.x = po2.x;
			}
			if (new2.x > po1.x && (pw2.x < po1.x ||
				(consider_touching && pw2.x == po1.x)))
			{
				new2.x = po1.x;
			}
		}
		/* Shrink top/bottom edges */
		if (po1.x < pw2.x && po2.x > pw1.x)
		{
			if (new1.y < po2.y && (pw1.y > po2.y ||
				(consider_touching && pw1.y == po2.y)))
			{
				new1.y = po2.y;
			}
			if (new2.y > po1.y && (pw2.y < po1.y ||
				(consider_touching && pw2.y == po1.y)))
			{
				new2.y = po1.y;
			}
		}
	}
	win_r->x = new1.x;
	win_r->width = new2.x - new1.x;
	win_r->y = new1.y;
	win_r->height = new2.y - new1.y;
}

static void unmaximize_fvwm_window(
	FvwmWindow *fw, cmdparser_context_t *pc)
{
	char	*cmd;
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
	get_relative_geometry(
		fw, &new_g, fw->fullscreen.was_maximized ?
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

	xasprintf(&cmd, "MoveToScreen %s", fw->m->si->name);
	execute_function_override_window(NULL, NULL, cmd, NULL, 0, fw);
	free(cmd);

	border_draw_decorations(
		fw, PART_ALL, (Scr.Hilite == fw), True, CLEAR_ALL, NULL, NULL);

	if (fw->fullscreen.is_shaded)
	{
		execute_function_override_window(
			NULL, NULL, "WindowShade on", NULL, 0, fw);

		fw->fullscreen.is_shaded = 0;
	}

	if (fw->fullscreen.is_iconified) {
		execute_function_override_window(
			NULL, NULL, "Iconify on", NULL, 0, fw);
		fw->fullscreen.is_iconified = 0;
	}

	/* Since the window's geometry will have been constrained already when
	 * coming out of fullscreen, we can always call this; either it's a
	 * noop or the window will be correctly decorated in the case of the
	 * window being restored from fullscreen to a non-maximized state.
	 */
	apply_decor_change(fw);
	EWMH_SetWMState(fw, True);
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
	position page;
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
	rectangle scr;
	rectangle new_g, bound;
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
				fscreen_scr_arg arg;
				arg.mouse_ev = NULL;
				is_screen_given = True;
				token = PeekToken(taction, &action);

				arg.name = token;

				FScreenGetScrRect(&arg, FSCREEN_BY_NAME,
					&scr.x, &scr.y, &scr.width, &scr.height);
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
			unmaximize_fvwm_window(fw, pc);
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
	get_page_offset_check_visible(&page.x, &page.y, fw);

	if (!is_screen_given)
	{
		fscreen_scr_arg fscr;

		fscr.xypos.x = fw->g.frame.x + fw->g.frame.width  / 2 - page.x;
		fscr.xypos.y = fw->g.frame.y + fw->g.frame.height / 2 - page.y;
		FScreenGetScrRect(
			&fscr, FSCREEN_XYPOS, &scr.x, &scr.y,
			&scr.width, &scr.height);
	}

	if (!ignore_working_area)
	{
		EWMH_GetWorkAreaIntersection(
			fw, &scr.x, &scr.y, &scr.width, &scr.height,
			EWMH_MAXIMIZE_MODE(fw));
	}
#if 0
	fprintf(stderr, "%s: page=(%d,%d), scr=(%d,%d, %dx%d)\n", __FUNCTION__,
		page.x, page.y, scr.x, scr.y, scr.width, scr.height);
#endif

	/* parse first parameter */
	val1_unit = scr.width;
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
		val1_unit = scr.width;
	}
	else if (token && StrEquals(token, "growleft"))
	{
		grow_left = True;
		val1 = 100;
		val1_unit = scr.width;
	}
	else if (token && StrEquals(token, "growright"))
	{
		grow_right = True;
		val1 = 100;
		val1_unit = scr.width;
	}
	else
	{
		if (GetOnePercentArgument(token, &val1, &val1_unit) == 0)
		{
			val1 = 100;
			val1_unit = scr.width;
		}
		else if (val1 < 0)
		{
			/* handle negative offsets */
			if (val1_unit == scr.width)
			{
				val1 = 100 + val1;
			}
			else
			{
				val1 = scr.width + val1;
			}
		}
	}

	/* parse second parameter */
	val2_unit = scr.height;
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
		val2_unit = scr.height;
	}
	else if (token && StrEquals(token, "growup"))
	{
		grow_up = True;
		val2 = 100;
		val2_unit = scr.height;
	}
	else if (token && StrEquals(token, "growdown"))
	{
		grow_down = True;
		val2 = 100;
		val2_unit = scr.height;
	}
	else
	{
		if (GetOnePercentArgument(token, &val2, &val2_unit) == 0)
		{
			val2 = 100;
			val2_unit = scr.height;
		}
		else if (val2 < 0)
		{
			/* handle negative offsets */
			if (val2_unit == scr.height)
			{
				val2 = 100 + val2;
			}
			else
			{
				val2 = scr.height + val2;
			}
		}
	}

#if 0
	fprintf(stderr, "%s: page=(%d,%d), scr=(%d,%d, %dx%d)\n", __FUNCTION__,
		page.x, page.y, scr.x, scr.y, scr.width, scr.height);
#endif

	if (do_forget == True)
	{
		fw->g.normal = fw->g.max;
		unmaximize_fvwm_window(fw, pc);
	}
	else if (IS_MAXIMIZED(fw) && !do_force_maximize)
	{
		unmaximize_fvwm_window(fw, pc);
	}
	else /* maximize */
	{
		/* handle command line arguments */
		if (grow_up || grow_down)
		{
			bound = new_g;
			if (grow_up)
			{
				bound.y = page.y + scr.y;
				bound.height = new_g.y + new_g.height - bound.y;
			}
			if (grow_down)
			{
				bound.height =
					page.y + scr.y + scr.height - bound.y;
			}
			grow_to_closest_type(fw, &new_g, bound, layers,
				SNAP_NONE, true);
		}
		else if (val2 > 0)
		{
			new_g.height = val2 * val2_unit / 100;
			new_g.y = page.y + scr.y;
		}
		if (grow_left || grow_right)
		{
			bound = new_g;
			if (grow_left)
			{
				bound.x = page.x + scr.x;
				bound.width = new_g.x + new_g.width - bound.x;
			}
			if (grow_right)
			{
				bound.width =
					page.x + scr.x + scr.width - bound.x;
			}
			grow_to_closest_type(fw, &new_g, bound, layers,
				SNAP_NONE, true);
		}
		else if (val1 >0)
		{
			new_g.width = val1 * val1_unit / 100;
			new_g.x = page.x + scr.x;
		}
		if (val1 == 0 && val2 == 0)
		{
			new_g.x = page.x + scr.x;
			new_g.y = page.y + scr.y;
			new_g.height = scr.height;
			new_g.width = scr.width;
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
	struct monitor	*m = (fw && fw->m) ? fw->m : monitor_get_current();

	/* keep a copy of the old geometry */
	normal_g = fw->g.normal;
	/* resize the window normally */
	was_resized = __resize_window(F_PASS_ARGS);
	if (was_resized == True)
	{
		/* set the new geometry as the maximized geometry and restore
		 * the old normal geometry */
		max_g = fw->g.normal;
		max_g.x -= m->virtual_scr.Vx;
		max_g.y -= m->virtual_scr.Vy;
		fw->g.normal = normal_g;
		/* and mark it as maximized */
		maximize_fvwm_window(fw, &max_g);
	}
	EWMH_SetWMState(fw, False);
	update_fvwm_monitor(fw);

	return;
}

void CMD_ResizeMoveMaximize(F_CMD_ARGS)
{
	rectangle normal_g;
	rectangle max_g;
	Bool was_resized;
	FvwmWindow *fw = exc->w.fw;
	struct monitor	*m = (fw && fw->m) ? fw->m : monitor_get_current();

	/* keep a copy of the old geometry */
	normal_g = fw->g.normal;
	/* resize the window normally */
	was_resized = resize_move_window(F_PASS_ARGS);
	if (was_resized == True)
	{
		/* set the new geometry as the maximized geometry and restore
		 * the old normal geometry */
		max_g = fw->g.normal;
		max_g.x -= m->virtual_scr.Vx;
		max_g.y -= m->virtual_scr.Vy;
		fw->g.normal = normal_g;
		/* and mark it as maximized */
		maximize_fvwm_window(fw, &max_g);
	}
	EWMH_SetWMState(fw, False);
	update_fvwm_monitor(fw);

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
		if (!IsRectangleOnThisPage(fw->m, &fw->g.frame,
		    fw->m->virtual_scr.CurrentDesk))
		{
			action = "";
			__move_window(F_PASS_ARGS, False, MOVE_PAGE);
			update_fvwm_monitor(fw);
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
		fw->Desk = fw->m->virtual_scr.CurrentDesk;
	}
	else
	{
		if (fw->Desk != fw->m->virtual_scr.CurrentDesk)
		{
			do_move_window_to_desk(fw, fw->m->virtual_scr.CurrentDesk);
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

		desk_add_fw(fw);
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
