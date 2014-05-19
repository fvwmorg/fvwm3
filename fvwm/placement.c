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
#include "libs/FScreen.h"
#include "libs/Grab.h"
#include "libs/Parse.h"
#include "libs/XResource.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "cursor.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "placement.h"
#include "geometry.h"
#include "update.h"
#include "style.h"
#include "move_resize.h"
#include "virtual.h"
#include "stack.h"
#include "ewmh.h"
#include "icons.h"
#include "add_window.h"

/* ---------------------------- local definitions -------------------------- */

#define MAX_NUM_PLACEMENT_ALGOS 31
#define CP_GET_NEXT_STEP 5

/* ---------------------------- local macros ------------------------------- */

#ifndef MIN
#define MIN(A,B) ((A)<(B)? (A):(B))
#endif
#ifndef MAX
#define MAX(A,B) ((A)>(B)? (A):(B))
#endif

#define NORMAL_PLACEMENT_PENALTY(p)      (p->normal)
#define ONTOP_PLACEMENT_PENALTY(p)       (p->ontop)
#define ICON_PLACEMENT_PENALTY(p)        (p->icon)
#define STICKY_PLACEMENT_PENALTY(p)      (p->sticky)
#define BELOW_PLACEMENT_PENALTY(p)       (p->below)
#define EWMH_STRUT_PLACEMENT_PENALTY(p)  (p->strut)

#define PERCENTAGE_99_PENALTY(p) (p->p99)
#define PERCENTAGE_95_PENALTY(p) (p->p95)
#define PERCENTAGE_85_PENALTY(p) (p->p85)
#define PERCENTAGE_75_PENALTY(p) (p->p75)

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

typedef enum
{
	PR_POS_NORMAL = 0,
	PR_POS_IGNORE_PPOS,
	PR_POS_USE_PPOS,
	PR_POS_IGNORE_USPOS,
	PR_POS_USE_USPOS,
	PR_POS_PLACE_AGAIN,
	PR_POS_CAPTURE,
	PR_POS_USPOS_OVERRIDE_SOS
} preason_pos_t;

typedef enum
{
	PR_SCREEN_CURRENT = 0,
	PR_SCREEN_STYLE,
	PR_SCREEN_X_RESOURCE_FVWMSCREEN,
	PR_SCREEN_IGNORE_CAPTURE
} preason_screen_t;

typedef enum
{
	PR_PAGE_CURRENT = 0,
	PR_PAGE_STYLE,
	PR_PAGE_X_RESOURCE_PAGE,
	PR_PAGE_IGNORE_CAPTURE,
	PR_PAGE_IGNORE_INVALID,
	PR_PAGE_STICKY
} preason_page_t;

typedef enum
{
	PR_DESK_CURRENT = 0,
	PR_DESK_STYLE,
	PR_DESK_X_RESOURCE_DESK,
	PR_DESK_X_RESOURCE_PAGE,
	PR_DESK_CAPTURE,
	PR_DESK_STICKY,
	PR_DESK_WINDOW_GROUP_LEADER,
	PR_DESK_WINDOW_GROUP_MEMBER,
	PR_DESK_TRANSIENT,
	PR_DESK_XPROP_XA_WM_DESKTOP
} preason_desk_t;

typedef struct
{
	struct
	{
		preason_pos_t reason;
		int x;
		int y;
		int algo;
		char *pl_position_string;
		unsigned do_not_manual_icon_placement : 1;
		unsigned do_adjust_off_screen : 1;
		unsigned do_adjust_off_page : 1;
		unsigned is_pl_position_string_invalid : 1;
		unsigned has_tile_failed : 1;
		unsigned has_manual_failed : 1;
		unsigned has_placement_failed : 1;
	} pos;
	struct
	{
		preason_screen_t reason;
		char *screen;
		rectangle g;
		unsigned was_modified_by_ewmh_workingarea : 1;
	} screen;
	struct
	{
		preason_page_t reason;
		int px;
		int py;
		unsigned do_switch_page : 1;
		unsigned do_honor_starts_on_page : 1;
		unsigned do_ignore_starts_on_page : 1;
	} page;
	struct
	{
		preason_desk_t reason;
		preason_desk_t sod_reason;
		int desk;
		unsigned do_switch_desk : 1;
	} desk;
} pl_reason_t;

typedef struct
{
	int desk;
	int page_x;
	int page_y;
	char *screen;
} pl_start_style_t;

typedef struct
{
	unsigned do_forbid_manual_placement : 1;
	unsigned do_honor_starts_on_page : 1;
	unsigned do_honor_starts_on_screen : 1;
	unsigned do_not_use_wm_placement : 1;
} pl_flags_t;

typedef float pl_penalty_t;

typedef enum
{
	PL_LOOP_END,
	PL_LOOP_CONT
} pl_loop_rc_t;

struct pl_arg_t;
struct pl_ret_t;

typedef struct
{
	/* If this funtion pointer is not NULL, use this function to return
	 * the desired position in a single call */
	pl_penalty_t (*get_pos_simple)(
		position *ret_p, struct pl_ret_t *ret,
		const struct pl_arg_t *arg);
	/* otherwise use these three in a loop */
	pl_loop_rc_t (*get_first_pos)(
		position *ret_p, struct pl_ret_t *ret,
		const struct pl_arg_t *arg);
	pl_loop_rc_t (*get_next_pos)(
		position *ret_p, struct pl_ret_t *ret,
		const struct pl_arg_t *arg, position hint_p);
	pl_penalty_t (*get_pos_penalty)(
		position *ret_hint_p, struct pl_ret_t *ret,
		const struct pl_arg_t *arg);
} pl_algo_t;

typedef struct pl_scratch_t
{
	const pl_penalty_struct *pp;
	const pl_percent_penalty_struct *ppp;
} pl_scratch_t;

typedef struct pl_arg_t
{
	const pl_algo_t *algo;
	const exec_context_t *exc;
	const window_style *style;
	pl_reason_t *reason;
	FvwmWindow *place_fw;
	pl_scratch_t *scratch;
	rectangle place_g;
	position place_p2;
	rectangle screen_g;
	position page_p1;
	position page_p2;
	position pdelta_p;
	struct
	{
		unsigned use_percent : 1;
		unsigned use_ewmh_dynamic_working_areapercent : 1;
		unsigned do_honor_starts_on_page : 1;
	} flags;
} pl_arg_t;

typedef struct pl_ret_t
{
	position best_p;
	pl_penalty_t best_penalty;
	struct
	{
		unsigned do_resize_too : 1;
	} flags;
} pl_ret_t;

/* ---------------------------- forward declarations ----------------------- */

static pl_loop_rc_t __pl_minoverlap_get_first_pos(
	position *ret_p, struct pl_ret_t *ret, const struct pl_arg_t *arg);
static pl_loop_rc_t __pl_minoverlap_get_next_pos(
	position *ret_p, struct pl_ret_t *ret, const struct pl_arg_t *arg,
	position hint_p);
static pl_penalty_t __pl_minoverlap_get_pos_penalty(
	position *ret_hint_p, struct pl_ret_t *ret, const pl_arg_t *arg);

static pl_penalty_t __pl_smart_get_pos_penalty(
	position *ret_hint_p, struct pl_ret_t *ret, const pl_arg_t *arg);

static pl_penalty_t __pl_position_get_pos_simple(
	position *ret_p, struct pl_ret_t *ret, const struct pl_arg_t *arg);

static pl_penalty_t __pl_cascade_get_pos_simple(
	position *ret_p, struct pl_ret_t *ret, const struct pl_arg_t *arg);

static pl_penalty_t __pl_manual_get_pos_simple(
	position *ret_p, struct pl_ret_t *ret, const struct pl_arg_t *arg);

/* ---------------------------- local variables ---------------------------- */

const pl_algo_t minoverlap_placement_algo =
{
	NULL,
	__pl_minoverlap_get_first_pos,
	__pl_minoverlap_get_next_pos,
	__pl_minoverlap_get_pos_penalty
};

const pl_algo_t smart_placement_algo =
{
	NULL,
	__pl_minoverlap_get_first_pos,
	__pl_minoverlap_get_next_pos,
	__pl_smart_get_pos_penalty
};

const pl_algo_t position_placement_algo =
{
	__pl_position_get_pos_simple
};

const pl_algo_t cascade_placement_algo =
{
	__pl_cascade_get_pos_simple
};

const pl_algo_t manual_placement_algo =
{
	__pl_manual_get_pos_simple
};

/* ---------------------------- exported variables (globals) --------------- */

const pl_penalty_struct default_pl_penalty =
{
	1,
	PLACEMENT_AVOID_ONTOP,
	PLACEMENT_AVOID_ICON,
	PLACEMENT_AVOID_STICKY,
	PLACEMENT_AVOID_BELOW,
	PLACEMENT_AVOID_EWMH_STRUT
};

const pl_percent_penalty_struct default_pl_percent_penalty =
{
	PLACEMENT_AVOID_COVER_99,
	PLACEMENT_AVOID_COVER_95,
	PLACEMENT_AVOID_COVER_85,
	PLACEMENT_AVOID_COVER_75
};

/* ---------------------------- local functions (PositionPlacement) -------- */

static pl_penalty_t __pl_position_get_pos_simple(
	position *ret_p, struct pl_ret_t *ret, const struct pl_arg_t *arg)
{
	char *spos;
	Bool fPointer;
	int n;
	int i;
	Bool is_under_mouse;

	is_under_mouse = False;
	spos = SGET_PLACEMENT_POSITION_STRING(*arg->style);
	if (spos == NULL || *spos == 0)
	{
		spos = DEFAULT_PLACEMENT_POSITION_STRING;
		i = 1;
	}
	else if (StrEquals(spos, "Center"))
	{
		spos = DEFAULT_PLACEMENT_POS_CENTER_STRING;
		i = 1;
	}
	else if (StrEquals(spos, "UnderMouse"))
	{
		spos = DEFAULT_PLACEMENT_POS_MOUSE_STRING;
		i = 1;
		is_under_mouse = True;
	}
	else
	{
		i = 0;
	}
	arg->reason->pos.pl_position_string = spos;
	for (n = -1; i < 2 && n < 2; i++)
	{
		fPointer = False;
		ret_p->x = 0;
		ret_p->y = 0;
		n = GetMoveArguments(
			&spos, arg->place_g.width, arg->place_g.height,
			&ret_p->x, &ret_p->y, NULL, &fPointer, False);
		spos = DEFAULT_PLACEMENT_POSITION_STRING;
		if (n < 2)
		{
			arg->reason->pos.is_pl_position_string_invalid = 1;
		}
	}
	if (n < 2)
	{
		/* bug */
		abort();
	}
	if (is_under_mouse)
	{
		/* TA:  20090218:  Try and keep the window on-screen if we
		 * can.
		 */

		/* TA:  20120316:  Imply the working-area when under the mouse -- this
		 * brings it in-line with making the EWMH working area the default.
		 * Note that "UnderMouse" is a special case, deliberately.  All other
		 * PositionPlacement commands are deliberately NOT subject to ewmhiwa
		 * options.
		 */
		EWMH_GetWorkAreaIntersection(
			arg->place_fw, (int *)&arg->screen_g.x, (int *)&arg->screen_g.y,
			(int *)&arg->screen_g.width,
			(int *)&arg->screen_g.height, EWMH_USE_WORKING_AREA);

		if (ret_p->x + arg->place_fw->g.frame.width > arg->screen_g.x
				+ arg->screen_g.width)
		{
			ret_p->x = (arg->screen_g.x + arg->screen_g.width) -
				arg->place_fw->g.frame.width;
		}
		if (ret_p->y + arg->place_fw->g.frame.height > arg->screen_g.y
				+ arg->screen_g.height)
		{
			ret_p->y = (arg->screen_g.y + arg->screen_g.height) -
				arg->place_fw->g.frame.height;
		}
	}
	/* Don't let the upper left corner be offscreen. */
	if (ret_p->x < arg->screen_g.x)
	{
		ret_p->x = arg->screen_g.x;
	}
	if (ret_p->y < arg->screen_g.y)
	{
		ret_p->y = arg->screen_g.y;
	}
	if (arg->flags.do_honor_starts_on_page)
	{
		ret_p->x -= arg->pdelta_p.x;
		ret_p->y -= arg->pdelta_p.y;
	}

	return 0;
}

/* ---------------------------- local functions (CascadePlacement)---------- */

static pl_penalty_t __pl_cascade_get_pos_simple(
	position *ret_p, struct pl_ret_t *ret, const struct pl_arg_t *arg)
{
	size_borders b;
	int w;
	FvwmWindow *t;

	t = (Scr.cascade_window != NULL) ? Scr.cascade_window : arg->place_fw;
	w = t->title_thickness;
	if (w == 0)
	{
		w = t->boundary_width;
	}
	if (w == 0)
	{
		w = PLACEMENT_FALLBACK_CASCADE_STEP;
	}

	if (Scr.cascade_window != NULL)
	{

		Scr.cascade_x += w;
		Scr.cascade_y += w;
		switch (GET_TITLE_DIR(t))
		{
		case DIR_S:
		case DIR_N:
			Scr.cascade_y += w;
			break;
		case DIR_E:
		case DIR_W:
			Scr.cascade_x += w;
			break;
		default:
			break;
		}
	}
	Scr.cascade_window = arg->place_fw;
	if (Scr.cascade_x > arg->screen_g.width / 2)
	{
		Scr.cascade_x = arg->place_fw->title_thickness;
	}
	if (Scr.cascade_y > arg->screen_g.height / 2)
	{
		Scr.cascade_y = 2 * arg->place_fw->title_thickness;
	}
	ret_p->x = Scr.cascade_x + arg->page_p1.x;
	ret_p->y = Scr.cascade_y + arg->page_p1.y;
	/* try to keep the window on the screen */
	get_window_borders(arg->place_fw, &b);
	if (ret_p->x + arg->place_g.width >= arg->page_p2.x)
	{
		ret_p->x = arg->page_p2.x - arg->place_g.width -
			b.total_size.width;
		Scr.cascade_x = arg->place_fw->title_thickness;
		switch (GET_TITLE_DIR(t))
		{
		case DIR_E:
		case DIR_W:
			Scr.cascade_x += arg->place_fw->title_thickness;
		default:
			break;
		}
	}
	if (ret_p->y + arg->place_g.height >= arg->page_p2.y)
	{
		ret_p->y = arg->page_p2.y - arg->place_g.height -
			b.total_size.height;
		Scr.cascade_y = arg->place_fw->title_thickness;
		switch (GET_TITLE_DIR(t))
		{
		case DIR_N:
		case DIR_S:
			Scr.cascade_y += arg->place_fw->title_thickness;
		default:
			break;
		}
	}

	/* the left and top sides are more important in huge windows */
	if (ret_p->x < arg->page_p1.x)
	{
		ret_p->x = arg->page_p1.x;
	}
	if (ret_p->y < arg->page_p1.y)
	{
		ret_p->y = arg->page_p1.y;
	}

	return 0;
}

/* ---------------------------- local functions (ManualPlacement)----------- */

static pl_penalty_t __pl_manual_get_pos_simple(
	position *ret_p, struct pl_ret_t *ret, const struct pl_arg_t *arg)
{
	ret_p->x = 0;
	ret_p->y = 0;
	if (GrabEm(CRS_POSITION, GRAB_NORMAL))
	{
		int DragWidth;
		int DragHeight;
		int mx;
		int my;

		/* Grabbed the pointer - continue */
		MyXGrabServer(dpy);
		if (
			XGetGeometry(
				dpy, FW_W(arg->place_fw), &JunkRoot, &JunkX,
				&JunkY, (unsigned int*)&DragWidth,
				(unsigned int*)&DragHeight,
				(unsigned int*)&JunkBW,
				(unsigned int*)&JunkDepth) == 0)
		{
			MyXUngrabServer(dpy);
			UngrabEm(GRAB_NORMAL);

			return -1;
		}
		SET_PLACED_BY_FVWM(arg->place_fw, 0);
		MyXGrabKeyboard(dpy);
		DragWidth = arg->place_g.width;
		DragHeight = arg->place_g.height;

		if (Scr.SizeWindow != None)
		{
			XMapRaised(dpy, Scr.SizeWindow);
		}
		FScreenGetScrRect(NULL, FSCREEN_GLOBAL, &mx, &my, NULL, NULL);
		if (__move_loop(
			    arg->exc, mx, my, DragWidth, DragHeight, &ret_p->x,
			    &ret_p->y, False, CRS_POSITION))
		{
			ret->flags.do_resize_too = 1;
		}
		if (Scr.SizeWindow != None)
		{
			XUnmapWindow(dpy, Scr.SizeWindow);
		}
		MyXUngrabKeyboard(dpy);
		MyXUngrabServer(dpy);
		UngrabEm(GRAB_NORMAL);
	}
	else
	{
		/* couldn't grab the pointer - better do something */
		XBell(dpy, 0);
		ret_p->x = 0;
		ret_p->y = 0;
		arg->reason->pos.has_manual_failed = 1;
	}
	if (arg->flags.do_honor_starts_on_page)
	{
		ret_p->x -= arg->pdelta_p.x;
		ret_p->y -= arg->pdelta_p.y;
	}

	return 0;
}

/* ---------------------------- local functions (MinoverlapPlacement) ------ */

/* MinoverlapPlacement by Anthony Martin <amartin@engr.csulb.edu>
 * This algorithm places a new window such that there is a minimum amount of
 * interference with other windows.  If it can place a window without any
 * interference, fine.  Otherwise, it places it so that the area of of
 * interference between the new window and the other windows is minimized */

static int __pl_minoverlap_get_next_x(const pl_arg_t *arg)
{
	FvwmWindow *other_fw;
	int xnew;
	int xtest;
	int stickyx;
	int stickyy;
	int start,i;
	int win_left;
	rectangle g;
	Bool rc;
	int x;
	int y;

	x = arg->place_g.x;
	y = arg->place_g.y;
	if (arg->flags.use_percent == 1)
	{
		start = 0;
	}
	else
	{
		start = CP_GET_NEXT_STEP;
	}

	/* Test window at far right of screen */
	xnew = arg->page_p2.x;
	xtest = arg->page_p2.x - arg->place_g.width;
	if (xtest > x)
	{
		xnew = xtest;
	}
	/* test the borders of the working area */
	xtest = arg->page_p1.x + Scr.Desktops->ewmh_working_area.x;
	if (xtest > x)
	{
		xnew = MIN(xnew, xtest);
	}
	xtest = arg->page_p1.x +
		(Scr.Desktops->ewmh_working_area.x +
		 Scr.Desktops->ewmh_working_area.width) -
		arg->place_g.width;
	if (xtest > x)
	{
		xnew = MIN(xnew, xtest);
	}
	/* Test the values of the right edges of every window */
	for (
		other_fw = Scr.FvwmRoot.next; other_fw != NULL;
		other_fw = other_fw->next)
	{
		if (
			other_fw == arg->place_fw ||
			(other_fw->Desk != arg->place_fw->Desk &&
			 !IS_STICKY_ACROSS_DESKS(other_fw)) ||
			IS_EWMH_DESKTOP(FW_W(other_fw)))
		{
			continue;
		}
		if (IS_STICKY_ACROSS_PAGES(other_fw))
		{
			stickyx = arg->pdelta_p.x;
			stickyy = arg->pdelta_p.y;
		}
		else
		{
			stickyx = 0;
			stickyy = 0;
		}
		if (IS_ICONIFIED(other_fw))
		{
			rc = get_visible_icon_geometry(other_fw, &g);
			if (rc == True && y < g.y + g.height - stickyy &&
			    g.y - stickyy < arg->place_g.height + y)
			{
				win_left = arg->page_p1.x + g.x - stickyx -
					arg->place_g.width;
				for (i = start; i <= CP_GET_NEXT_STEP; i++)
				{
					xtest = win_left + g.width *
						(CP_GET_NEXT_STEP - i) /
						CP_GET_NEXT_STEP;
					if (xtest > x)
					{
						xnew = MIN(xnew, xtest);
					}
				}
				win_left = arg->page_p1.x + g.x - stickyx;
				for (i = start; i <= CP_GET_NEXT_STEP; i++)
				{
					xtest = (win_left) + g.width * i /
						CP_GET_NEXT_STEP;
					if (xtest > x)
					{
						xnew = MIN(xnew, xtest);
					}
				}
			}
		}
		else if (
			y < other_fw->g.frame.height + other_fw->g.frame.y -
			stickyy &&
			other_fw->g.frame.y - stickyy <
			arg->place_g.height + y &&
			arg->page_p1.x < other_fw->g.frame.width +
			other_fw->g.frame.x - stickyx &&
			other_fw->g.frame.x - stickyx < arg->page_p2.x)
		{
			win_left =
				other_fw->g.frame.x - stickyx -
				arg->place_g.width;
			for (i = start; i <= CP_GET_NEXT_STEP; i++)
			{
				xtest = win_left + other_fw->g.frame.width *
					(CP_GET_NEXT_STEP - i) /
					CP_GET_NEXT_STEP;
				if (xtest > x)
				{
					xnew = MIN(xnew, xtest);
				}
			}
			win_left = other_fw->g.frame.x - stickyx;
			for (i = start; i <= CP_GET_NEXT_STEP; i++)
			{
				xtest = win_left + other_fw->g.frame.width *
					i / CP_GET_NEXT_STEP;
				if (xtest > x)
				{
					xnew = MIN(xnew, xtest);
				}
			}
		}
	}

	return xnew;
}

static int __pl_minoverlap_get_next_y(const pl_arg_t *arg)
{
	FvwmWindow *other_fw;
	int ynew;
	int ytest;
	int stickyy;
	int win_top;
	int start;
	int i;
	rectangle g;
	int y;

	y = arg->place_g.y;
	if (arg->flags.use_percent == 1)
	{
		start = 0;
	}
	else
	{
		start = CP_GET_NEXT_STEP;
	}

	/* Test window at far bottom of screen */
	ynew = arg->page_p2.y;
	ytest = arg->page_p2.y - arg->place_g.height;
	if (ytest > y)
	{
		ynew = ytest;
	}
	/* test the borders of the working area */
	ytest = arg->page_p1.y + Scr.Desktops->ewmh_working_area.y;
	if (ytest > y)
	{
		ynew = MIN(ynew, ytest);
	}
	ytest = arg->screen_g.y +
		(Scr.Desktops->ewmh_working_area.y +
		 Scr.Desktops->ewmh_working_area.height) -
		arg->place_g.height;
	if (ytest > y)
	{
		ynew = MIN(ynew, ytest);
	}
	/* Test the values of the bottom edge of every window */
	for (
		other_fw = Scr.FvwmRoot.next; other_fw != NULL;
		other_fw = other_fw->next)
	{
		if (
			other_fw == arg->place_fw ||
			(
				other_fw->Desk != arg->place_fw->Desk &&
				!IS_STICKY_ACROSS_DESKS(other_fw)) ||
			IS_EWMH_DESKTOP(FW_W(other_fw)))
		{
			continue;
		}

		if (IS_STICKY_ACROSS_PAGES(other_fw))
		{
			stickyy = arg->pdelta_p.y;
		}
		else
		{
			stickyy = 0;
		}

		if (IS_ICONIFIED(other_fw))
		{
			get_visible_icon_geometry(other_fw, &g);
			win_top = g.y - stickyy;
			for (i = start; i <= CP_GET_NEXT_STEP; i++)
			{
				ytest =
					win_top + g.height * i /
					CP_GET_NEXT_STEP;
				if (ytest > y)
				{
					ynew = MIN(ynew, ytest);
				}
			}
			win_top = g.y - stickyy - arg->place_g.height;
			for (i = start; i <= CP_GET_NEXT_STEP; i++)
			{
				ytest =
					win_top + g.height *
					(CP_GET_NEXT_STEP - i) /
					CP_GET_NEXT_STEP;
				if (ytest > y)
				{
					ynew = MIN(ynew, ytest);
				}
			}
		}
		else
		{
			win_top = other_fw->g.frame.y - stickyy;
			for (i = start; i <= CP_GET_NEXT_STEP; i++)
			{
				ytest =
					win_top + other_fw->g.frame.height *
					i / CP_GET_NEXT_STEP;
				if (ytest > y)
				{
					ynew = MIN(ynew, ytest);
				}
			}
			win_top = other_fw->g.frame.y - stickyy -
				arg->place_g.height;
			for (i = start; i <= CP_GET_NEXT_STEP; i++)
			{
				ytest =
					win_top + other_fw->g.frame.height *
					(CP_GET_NEXT_STEP - i) /
					CP_GET_NEXT_STEP;
				if (ytest > y)
				{
					ynew = MIN(ynew, ytest);
				}
			}
		}
	}

	return ynew;
}

static pl_loop_rc_t __pl_minoverlap_get_first_pos(
	position *ret_p, struct pl_ret_t *ret, const pl_arg_t *arg)
{
	/* top left corner of page */
	ret_p->x = arg->page_p1.x;
	ret_p->y = arg->page_p1.y;

	return PL_LOOP_CONT;
}

static pl_loop_rc_t __pl_minoverlap_get_next_pos(
	position *ret_p, struct pl_ret_t *ret, const struct pl_arg_t *arg,
	position hint_p)
{
	ret_p->x = arg->place_g.x;
	ret_p->y = arg->place_g.y;
	if (ret_p->x + arg->place_g.width <= arg->page_p2.x)
	{
		/* try next x */
		ret_p->x = __pl_minoverlap_get_next_x(arg);
		ret_p->y = arg->place_g.y;
	}
	if (ret_p->x + arg->place_g.width > arg->page_p2.x)
	{
		/* out of room in x direction. Try next y. Reset x.*/
		ret_p->x = arg->page_p1.x;
		ret_p->y = __pl_minoverlap_get_next_y(arg);
	}
	if (ret_p->y + arg->place_g.height > arg->page_p2.y)
	{
		/* PageBottom */
		return PL_LOOP_END;
	}

	return PL_LOOP_CONT;
}

static pl_penalty_t __pl_minoverlap_get_avoidance_penalty(
	const pl_arg_t *arg, FvwmWindow *other_fw, const rectangle *other_g)
{
	pl_penalty_t anew;
	pl_penalty_t avoidance_factor;
	position other_p2;
	const pl_penalty_struct *opp;
	const pl_percent_penalty_struct *oppp;

	opp = (arg->scratch->pp != 0 && 0) ? arg->scratch->pp :
		&other_fw->pl_penalty;
	oppp = (arg->scratch->ppp != 0 && 0) ? arg->scratch->ppp :
		&other_fw->pl_percent_penalty;
	other_p2.x = other_g->x + other_g->width;
	other_p2.y = other_g->y + other_g->height;
	{
		long x1 = MAX(arg->place_g.x, other_g->x);
		long x2 = MIN(arg->place_p2.x, other_p2.x);
		long y1 = MAX(arg->place_g.y, other_g->y);
		long y2 = MIN(arg->place_p2.y, other_p2.y);

		/* overlapping area in pixels (windows are guaranteed to
		 * overlap when this function is called) */
		anew = (x2 - x1) * (y2 - y1);
	}
	if (IS_ICONIFIED(other_fw))
	{
		avoidance_factor = ICON_PLACEMENT_PENALTY(opp);
	}
	else if (compare_window_layers(other_fw, arg->place_fw) > 0)
	{
		avoidance_factor = ONTOP_PLACEMENT_PENALTY(opp);
	}
	else if (compare_window_layers(other_fw, arg->place_fw) < 0)
	{
		avoidance_factor = BELOW_PLACEMENT_PENALTY(opp);
	}
	else if (
		IS_STICKY_ACROSS_PAGES(other_fw) ||
		IS_STICKY_ACROSS_DESKS(other_fw))
	{
		avoidance_factor = STICKY_PLACEMENT_PENALTY(opp);
	}
	else
	{
		avoidance_factor = NORMAL_PLACEMENT_PENALTY(opp);
	}
	if (arg->flags.use_percent == 1)
	{
		pl_penalty_t cover_factor;
		long other_area;
		long place_area;

		other_area = other_g->width * other_g->height;
		place_area = arg->place_g.width * arg->place_g.height;
		cover_factor = 0;
		if (other_area != 0 && place_area != 0)
		{
			anew = 100 * MAX(anew / other_area, anew / place_area);
			if (anew >= 99)
			{
				cover_factor = PERCENTAGE_99_PENALTY(oppp);
			}
			else if (anew > 94)
			{
				cover_factor = PERCENTAGE_95_PENALTY(oppp);
			}
			else if (anew > 84)
			{
				cover_factor = PERCENTAGE_85_PENALTY(oppp);
			}
			else if (anew > 74)
			{
				cover_factor = PERCENTAGE_75_PENALTY(oppp);
			}
		}
		if (avoidance_factor >= 1)
		{
			avoidance_factor += cover_factor;
		}
	}
	if (
		arg->flags.use_ewmh_dynamic_working_areapercent == 1 &&
		DO_EWMH_IGNORE_STRUT_HINTS(other_fw) == 0 &&
		(
			other_fw->dyn_strut.left > 0 ||
			other_fw->dyn_strut.right > 0 ||
			other_fw->dyn_strut.top > 0 ||
			other_fw->dyn_strut.bottom > 0))
	{
		const pl_penalty_struct *mypp;

		mypp = (arg->scratch->pp != 0 && 0) ? arg->scratch->pp :
			&arg->place_fw->pl_penalty;
		/* if we intersect a window which reserves space */
		avoidance_factor += (avoidance_factor >= 1) ?
			EWMH_STRUT_PLACEMENT_PENALTY(mypp) : 0;
	}
	anew *= avoidance_factor;

	return anew;
}

static pl_penalty_t __pl_minoverlap_get_pos_penalty(
	position *ret_hint_p, struct pl_ret_t *ret, const struct pl_arg_t *arg)
{
	FvwmWindow *other_fw;
	pl_penalty_t penalty;
	size_borders b;

	penalty = 0;
	for (
		other_fw = Scr.FvwmRoot.next; other_fw != NULL;
		other_fw = other_fw->next)
	{
		rectangle other_g;
		get_window_borders(other_fw, &b);

		if (
			arg->place_fw == other_fw ||
			IS_EWMH_DESKTOP(FW_W(other_fw)))
		{
			continue;
		}
		/*  RBW - account for sticky windows...  */
		if (
			other_fw->Desk != arg->place_fw->Desk &&
			IS_STICKY_ACROSS_DESKS(other_fw) == 0)
		{
			continue;
		}
		(void)get_visible_window_or_icon_geometry(other_fw, &other_g);
		if (IS_STICKY_ACROSS_PAGES(other_fw))
		{
			other_g.x -= arg->pdelta_p.x;
			other_g.y -= arg->pdelta_p.y;
		}
		if (
			arg->place_g.x < other_g.x + other_g.width &&
			arg->place_p2.x > other_g.x &&
			arg->place_g.y < other_g.y + other_g.height &&
			arg->place_p2.y > other_g.y)
		{
			pl_penalty_t anew;

			anew = __pl_minoverlap_get_avoidance_penalty(
				arg, other_fw, &other_g);
			penalty += anew;
			if (
				penalty > ret->best_penalty &&
				ret->best_penalty != -1)
			{
				/* TA:  20091230:  Fix over-zealous penalties
				 * by explicitly forcing the window on-screen
				 * here.  The y-axis is only affected here,
				 * due to how the xoffset calculations happen
				 * prior to setting the x-axis.  When we get
				 * penalties which are "over-zealous" -- and
				 * by not taking into account the size of the
				 * window borders, the window was being placed
				 * off screen.
				 */
				if (ret->best_p.y + arg->place_g.height > arg->page_p2.y)
				{
					ret->best_p.y =
						(arg->page_p2.y -
						arg->place_g.height -
						b.total_size.height);

					ret->best_penalty = 0;
					penalty = 0;
				}

				/* stop looking; the penalty is too high */
				return penalty;
			}
		}
	}
	/* now handle the working area */
	{
		const pl_penalty_struct *mypp;

		mypp = (arg->scratch->pp != 0 && 0) ? arg->scratch->pp :
			&arg->place_fw->pl_penalty;
		if (arg->flags.use_ewmh_dynamic_working_areapercent == 1)
		{
			penalty += EWMH_STRUT_PLACEMENT_PENALTY(mypp) *
				EWMH_GetStrutIntersection(
					arg->place_g.x, arg->place_g.y,
					arg->place_p2.x, arg->place_p2.y,
					arg->flags.use_percent);
		}
		else
		{
			/* EWMH_USE_DYNAMIC_WORKING_AREA, count the base strut
			 */
			penalty +=
				EWMH_STRUT_PLACEMENT_PENALTY(mypp) *
				EWMH_GetBaseStrutIntersection(
					arg->place_g.x, arg->place_g.y,
					arg->place_p2.x, arg->place_p2.y,
					arg->flags.use_percent);
		}
	}

	return penalty;
}

/* ---------------------------- local functions (SmartPlacement) ----------- */

static pl_penalty_t __pl_smart_get_pos_penalty(
	position *ret_hint_p, struct pl_ret_t *ret, const struct pl_arg_t *arg)
{
	pl_penalty_t p;

	arg->scratch->pp = &default_pl_penalty;
	arg->scratch->ppp = &default_pl_percent_penalty;
	p = __pl_minoverlap_get_pos_penalty(ret_hint_p, ret, arg);
	if (p != 0)
	{
		p = -1;
	}

	return p;
}

/* ---------------------------- local functions ---------------------------- */

static int placement_loop(pl_ret_t *ret, pl_arg_t *arg)
{
	position next_p;
	pl_penalty_t penalty;
	pl_loop_rc_t loop_rc;

	if (arg->algo->get_pos_simple != NULL)
	{
		position pos;

		penalty = arg->algo->get_pos_simple(&pos, ret, arg);
		arg->place_g.x = pos.x;
		arg->place_g.y = pos.y;
		ret->best_penalty = penalty;
		ret->best_p.x = pos.x;
		ret->best_p.y = pos.y;
		loop_rc = PL_LOOP_END;
	}
	else
	{
		loop_rc = arg->algo->get_first_pos(&next_p, ret, arg);
		arg->place_g.x = next_p.x;
		arg->place_g.y = next_p.y;
		ret->best_p.x = next_p.x;
		ret->best_p.y = next_p.y;
	}
	while (loop_rc != PL_LOOP_END)
	{
		position hint_p;
		pl_scratch_t scratch;

		memset(&scratch, 0, sizeof(scratch));
		arg->scratch = &scratch;
		arg->place_p2.x = arg->place_g.x + arg->place_g.width;
		arg->place_p2.y = arg->place_g.y + arg->place_g.height;
		hint_p.x = arg->place_g.x;
		hint_p.y = arg->place_g.y;
		penalty = arg->algo->get_pos_penalty(&hint_p, ret, arg);
		/* I've added +0.0001 because with my machine the < test fail
		 * with certain *equal* float numbers! */
		if (
			penalty >= 0 &&
			(
				ret->best_penalty < 0 ||
				penalty + 0.0001 < ret->best_penalty))
		{
			ret->best_p.x = arg->place_g.x;
			ret->best_p.y = arg->place_g.y;
			ret->best_penalty = penalty;
		}
		if (penalty == 0)
		{
			break;
		}
		loop_rc = arg->algo->get_next_pos(&next_p, ret, arg, hint_p);
		arg->place_g.x = next_p.x;
		arg->place_g.y = next_p.y;
	}
	if (ret->best_penalty < 0)
	{
		ret->best_penalty = -1;
	}

	return (ret->best_penalty == -1) ? -1 : 0;
}

static void __place_get_placement_flags(
	pl_flags_t *ret_flags, FvwmWindow *fw, window_style *pstyle,
	initial_window_options_t *win_opts, int mode, pl_reason_t *reason)
{
	Bool override_ppos;
	Bool override_uspos;
	Bool has_ppos = False;
	Bool has_uspos = False;

	/* Windows use the position hint if these conditions are met:
	 *
	 *  The program specified a USPosition hint and it is not overridden
	 *  with the No(Transient)USPosition style.
	 *
	 * OR
	 *
	 *  The program specified a PPosition hint and it is not overridden
	 *  with the No(Transient)PPosition style.
	 *
	 * Windows without a position hint are placed using wm placement.
	 */
	if (IS_TRANSIENT(fw))
	{
		override_ppos = SUSE_NO_TRANSIENT_PPOSITION(&pstyle->flags);
		override_uspos = SUSE_NO_TRANSIENT_USPOSITION(&pstyle->flags);
	}
	else
	{
		override_ppos = SUSE_NO_PPOSITION(&pstyle->flags);
		override_uspos = SUSE_NO_USPOSITION(&pstyle->flags);
	}
	if (fw->hints.flags & PPosition)
	{
		if (!override_ppos)
		{
			has_ppos = True;
			reason->pos.reason = PR_POS_USE_PPOS;
		}
		else
		{
			reason->pos.reason = PR_POS_IGNORE_PPOS;
		}
	}
	if (fw->hints.flags & USPosition)
	{
		if (!override_uspos)
		{
			has_uspos = True;
			reason->pos.reason = PR_POS_USE_USPOS;
		}
		else if (reason->pos.reason != PR_POS_USE_PPOS)
		{
			reason->pos.reason = PR_POS_IGNORE_USPOS;
		}
	}
	if (mode == PLACE_AGAIN)
	{
		ret_flags->do_not_use_wm_placement = 0;
		reason->pos.reason = PR_POS_PLACE_AGAIN;
	}
	else if (has_ppos || has_uspos)
	{
		ret_flags->do_not_use_wm_placement = 1;
	}
	else if (win_opts->flags.do_override_ppos)
	{
		ret_flags->do_not_use_wm_placement = 1;
		reason->pos.reason = PR_POS_CAPTURE;
	}
	else if (!ret_flags->do_honor_starts_on_page &&
		 fw->wmhints && (fw->wmhints->flags & StateHint) &&
		 fw->wmhints->initial_state == IconicState)
	{
		ret_flags->do_forbid_manual_placement = 1;
		reason->pos.do_not_manual_icon_placement = 1;
	}

	return;
}

static int __add_algo(
	const pl_algo_t **algos, int num_algos, const pl_algo_t *new_algo)
{
	if (num_algos >= MAX_NUM_PLACEMENT_ALGOS)
	{
		return MAX_NUM_PLACEMENT_ALGOS;
	}
	algos[num_algos] = new_algo;
	num_algos++;

	return num_algos;
}

static int __place_get_wm_pos(
	const exec_context_t *exc, window_style *pstyle, rectangle *attr_g,
	pl_flags_t flags, rectangle screen_g, pl_start_style_t start_style,
	int mode, initial_window_options_t *win_opts, pl_reason_t *reason,
	int pdeltax, int pdeltay)
{
	const pl_algo_t *algos[MAX_NUM_PLACEMENT_ALGOS + 1];
	int num_algos;
	unsigned int placement_mode = SPLACEMENT_MODE(&pstyle->flags);
	pl_arg_t arg;
	pl_ret_t ret;
	int i;

	/* BEGIN init placement agrs and ret */
	memset(&arg, 0, sizeof(arg));
	arg.exc = exc;
	arg.style = pstyle;
	arg.reason = reason;
	arg.place_fw = exc->w.fw;
	arg.place_g = arg.place_fw->g.frame;
	arg.screen_g = screen_g;
	arg.page_p1.x = arg.screen_g.x - pdeltax;
	arg.page_p1.y = arg.screen_g.y - pdeltay;
	arg.page_p2.x = arg.page_p1.x + screen_g.width;
	arg.page_p2.y = arg.page_p1.y + screen_g.height;
	arg.pdelta_p.x = pdeltax;
	arg.pdelta_p.y = pdeltay;
	arg.flags.use_percent = 0;
	arg.flags.do_honor_starts_on_page = flags.do_honor_starts_on_page;
	if (SEWMH_PLACEMENT_MODE(&pstyle->flags) == EWMH_USE_WORKING_AREA)
	{
		arg.flags.use_ewmh_dynamic_working_areapercent = 1;
	}
	memset(&ret, 0, sizeof(ret));
	ret.best_penalty = -1.0;
	/* END init placement agrs and ret */
	/* override if manual placement happens */
	SET_PLACED_BY_FVWM(arg.place_fw, 1);
	if (flags.do_forbid_manual_placement)
	{
		switch (placement_mode)
		{
		case PLACE_MANUAL:
		case PLACE_MANUAL_B:
			placement_mode = PLACE_CASCADE;
			break;
		case PLACE_TILEMANUAL:
			placement_mode = PLACE_TILECASCADE;
			break;
		default:
			break;
		}
	}
	reason->pos.algo = placement_mode;
	/* first, try various "smart" placement */
	num_algos = 0;
	switch (placement_mode)
	{
	case PLACE_POSITION:
		num_algos = __add_algo(
			algos, num_algos, &position_placement_algo);
		break;
	case PLACE_TILEMANUAL:
		num_algos = __add_algo(
			algos, num_algos, &smart_placement_algo);
		num_algos = __add_algo(
			algos, num_algos, &manual_placement_algo);
		break;
	case PLACE_MINOVERLAPPERCENT:
		arg.flags.use_percent = 1;
		/* fall through */
	case PLACE_MINOVERLAP:
		num_algos = __add_algo(
			algos, num_algos, &minoverlap_placement_algo);
		break;
	case PLACE_TILECASCADE:
		num_algos = __add_algo(
			algos, num_algos, &smart_placement_algo);
		num_algos = __add_algo(
			algos, num_algos, &cascade_placement_algo);
		break;
	case PLACE_MANUAL:
	case PLACE_MANUAL_B:
		num_algos = __add_algo(
			algos, num_algos, &manual_placement_algo);
		break;
	case PLACE_CASCADE:
	case PLACE_CASCADE_B:
		num_algos = __add_algo(
			algos, num_algos, &cascade_placement_algo);
		break;
	default:
		/* can't happen */
		break;
	}
	/* try all the placement algorithms */
	for (i = 0 ; ret.best_penalty < 0 && i < num_algos; i++)
	{
		arg.algo = algos[i];
		placement_loop(&ret, &arg);
	}
	if (ret.best_penalty >= 0)
	{
		/* placement succed */
		attr_g->x = ret.best_p.x;
		attr_g->y = ret.best_p.y;
	}
	else
	{
		/* fall back to default position */
		attr_g->x = 0;
		attr_g->y = 0;
		reason->pos.has_placement_failed = 1;
	}

	return ret.flags.do_resize_too;
}

static int __place_get_nowm_pos(
	const exec_context_t *exc, window_style *pstyle, rectangle *attr_g,
	pl_flags_t flags, rectangle screen_g, pl_start_style_t start_style,
	int mode, initial_window_options_t *win_opts, pl_reason_t *reason,
	int pdeltax, int pdeltay)
{
	FvwmWindow *fw = exc->w.fw;
	size_borders b;

	if (!win_opts->flags.do_override_ppos)
	{
		SET_PLACED_BY_FVWM(fw, False);
	}
	/* the USPosition was specified, or the window is a transient, or it
	 * starts iconic so place it automatically */
	if (
		SUSE_START_ON_SCREEN(&pstyle->flags) &&
		flags.do_honor_starts_on_screen)
	{
		/* If StartsOnScreen has been given for a window, translate its
		 * USPosition so that it is relative to that particular screen.
		 *  If we don't do this, then a geometry would completely
		 * cancel the effect of the StartsOnScreen style. However, some
		 * applications try to remember their position.  This would
		 * break if these were translated to screen coordinates.  There
		 * is no reliable way to do it.  Currently, if the desired
		 * place does not intersect the target screen, we assume the
		 * window position must be adjusted to the screen origin. So
		 * there are two ways to get a window to pop up on a particular
		 * Xinerama screen.  1: The intuitive way giving a geometry
		 * hint relative to the desired screen's 0,0 along with the
		 * appropriate StartsOnScreen style (or *wmscreen resource), or
		 * 2: Do NOT specify a Xinerama screen (or specify it to be
		 * 'g') and give the geometry hint in terms of the global
		 * screen. */
		int mangle_screen = FScreenFetchMangledScreenFromUSPosHints(
			&(fw->hints));
		if (mangle_screen != 0)
		{
			/* whoever set this hint knew exactly what he was
			 * doing; so ignore the StartsOnScreen style */
			flags.do_honor_starts_on_screen = 0;
			reason->pos.reason = PR_POS_USPOS_OVERRIDE_SOS;
		}
		if (attr_g->x + attr_g->width < screen_g.x ||
			 attr_g->x >= screen_g.x + screen_g.width ||
			 attr_g->y + attr_g->height < screen_g.y ||
			 attr_g->y >= screen_g.y + screen_g.height)
		{
			/* desired coordinates do not intersect the target
			 * screen.  Let's assume the application specified
			 * global coordinates and translate them to the screen.
			 */
			fscreen_scr_arg	 arg;
			arg.mouse_ev = NULL;
			arg.name = start_style.screen;
			FScreenTranslateCoordinates(
				&arg, FSCREEN_BY_NAME, NULL, FSCREEN_GLOBAL,
				&attr_g->x, &attr_g->y);
			reason->pos.do_adjust_off_screen = 1;
		}
	}
	/* If SkipMapping, and other legalities are observed, adjust for
	 * StartsOnPage. */
	if (DO_NOT_SHOW_ON_MAP(fw) && flags.do_honor_starts_on_page &&
	    (!IS_TRANSIENT(fw) ||
	     SUSE_START_ON_PAGE_FOR_TRANSIENT(&pstyle->flags))
#if 0
	    /* dv 08-Jul-2003:  Do not use this.  Instead, force the window on
	     * the requested page even if the application requested a different
	     * position. */
	    && (SUSE_NO_PPOSITION(&pstyle->flags) ||
		!(fw->hints.flags & PPosition))
	    /* dv 08-Jul-2003:  This condition is always true because we
	     * already checked for flags.do_honor_starts_on_page above. */
	    /*  RBW - allow StartsOnPage to go through, even if iconic. */
	    && ((!(fw->wmhints && (fw->wmhints->flags & StateHint) &&
		   fw->wmhints->initial_state == IconicState))
		|| flags.do_honor_starts_on_page)
#endif
		)
	{
		int old_x;
		int old_y;

		old_x = attr_g->x;
		old_y = attr_g->y;
		/* We're placing a SkipMapping window - either capturing one
		 * that's previously been mapped, or overriding USPosition - so
		 * what we have here is its actual untouched coordinates. In
		 * case it was a StartsOnPage window, we have to 1) convert the
		 * existing x,y offsets relative to the requested page (i.e.,
		 * as though there were only one page, no virtual desktop),
		 * then 2) readjust relative to the current page. */
		if (attr_g->x < 0)
		{
			attr_g->x += Scr.MyDisplayWidth;
		}
		attr_g->x %= Scr.MyDisplayWidth;
		attr_g->x -= pdeltax;
		/* Noticed a quirk here. With some apps (e.g., xman), we find
		 * the placement has moved 1 pixel away from where we
		 * originally put it when we come through here.  Why is this
		 * happening? Probably attr_backup.border_width, try xclock
		 * -borderwidth 100 */
		if (attr_g->y < 0)
		{
			attr_g->y += Scr.MyDisplayHeight;
		}
		attr_g->y %= Scr.MyDisplayHeight;
		attr_g->y -= pdeltay;
		if (attr_g->x != old_x || attr_g->y != old_y)
		{
			reason->pos.do_adjust_off_page = 1;
		}
	}
	/* put it where asked, mod title bar */
	/* if the gravity is towards the top, move it by the title height */
	{
		rectangle final_g;
		int gravx;
		int gravy;

		gravity_get_offsets(fw->hints.win_gravity, &gravx, &gravy);
		final_g.x = attr_g->x + gravx * fw->attr_backup.border_width;
		final_g.y = attr_g->y + gravy * fw->attr_backup.border_width;
		/* Virtually all applications seem to share a common bug: they
		 * request the top left pixel of their *border* as their origin
		 * instead of the top left pixel of their client window, e.g.
		 * 'xterm -g +0+0' creates an xterm that tries to map at (0 0)
		 * although its border (width 1) would not be visible if it ran
		 * under plain X.  It should have tried to map at (1 1)
		 * instead.  This clearly violates the ICCCM, but trying to
		 * change this is like tilting at windmills.  So we have to add
		 * the border width here. */
		final_g.x += fw->attr_backup.border_width;
		final_g.y += fw->attr_backup.border_width;
		final_g.width = 0;
		final_g.height = 0;
		if (mode == PLACE_INITIAL)
		{
			get_window_borders(fw, &b);
			gravity_resize(
				fw->hints.win_gravity, &final_g,
				b.total_size.width, b.total_size.height);
		}
		attr_g->x = final_g.x;
		attr_g->y = final_g.y;
	}

	return 0;
}

/* Handles initial placement and sizing of a new window
 *
 * Return value:
 *
 *   0 = window lost
 *   1 = OK
 *   2 = OK, window must be resized too */
static int __place_window(
	const exec_context_t *exc, window_style *pstyle, rectangle *attr_g,
	pl_start_style_t start_style, int mode,
	initial_window_options_t *win_opts, pl_reason_t *reason)
{
	FvwmWindow *t;
	int is_skipmapping_forbidden;
	int px = 0;
	int py = 0;
	int pdeltax = 0;
	int pdeltay = 0;
	rectangle screen_g;
	int rc = 0;
	pl_flags_t flags;
	extern Bool Restarting;
	FvwmWindow *fw = exc->w.fw;

	memset(&flags, 0, sizeof(flags));

	/* Select a desk to put the window on (in list of priority):
	 * 1. Sticky Windows stay on the current desk.
	 * 2. Windows specified with StartsOnDesk go where specified
	 * 3. Put it on the desk it was on before the restart.
	 * 4. Transients go on the same desk as their parents.
	 * 5. Window groups stay together (if the KeepWindowGroupsOnDesk style
	 * is used). */

	/* Let's get the StartsOnDesk/Page tests out of the way first. */
	if (
		SUSE_START_ON_DESK(&pstyle->flags) ||
		SUSE_START_ON_SCREEN(&pstyle->flags))
	{
		flags.do_honor_starts_on_page = 1;
		flags.do_honor_starts_on_screen = 1;
		/*
		 * Honor the flag unless...
		 * it's a restart or recapture, and that option's disallowed...
		 */
		if (win_opts->flags.do_override_ppos &&
		    (Restarting || (Scr.flags.are_windows_captured)) &&
		    !SRECAPTURE_HONORS_STARTS_ON_PAGE(&pstyle->flags))
		{
			flags.do_honor_starts_on_page = 0;
			flags.do_honor_starts_on_screen = 0;
			reason->page.reason =
				(preason_page_t)PR_PAGE_IGNORE_CAPTURE;
			reason->page.do_ignore_starts_on_page = 1;
			reason->screen.reason =
				(preason_screen_t)PR_PAGE_IGNORE_CAPTURE;
		}
		/*
		 * it's a cold start window capture, and that's disallowed...
		 */
		if (win_opts->flags.do_override_ppos &&
		    (!Restarting && !(Scr.flags.are_windows_captured)) &&
		    !SCAPTURE_HONORS_STARTS_ON_PAGE(&pstyle->flags))
		{
			flags.do_honor_starts_on_page = 0;
			flags.do_honor_starts_on_screen = 0;
			reason->page.reason = PR_PAGE_IGNORE_CAPTURE;
			reason->page.do_ignore_starts_on_page = 1;
			reason->screen.reason =
				(preason_screen_t)PR_PAGE_IGNORE_CAPTURE;
		}
		/*
		 * it's ActivePlacement and SkipMapping, and that's disallowed.
		 */
		switch (SPLACEMENT_MODE(&pstyle->flags))
		{
		case PLACE_MANUAL:
		case PLACE_MANUAL_B:
		case PLACE_TILEMANUAL:
			is_skipmapping_forbidden =
				!SMANUAL_PLACEMENT_HONORS_STARTS_ON_PAGE(
					&pstyle->flags);
			break;
		default:
			is_skipmapping_forbidden = 0;
			break;
		}
		if (win_opts->flags.do_override_ppos ||
		    !DO_NOT_SHOW_ON_MAP(fw))
		{
			is_skipmapping_forbidden = 0;
		}
		if (is_skipmapping_forbidden == 1)
		{
			flags.do_honor_starts_on_page = 0;
			reason->page.reason = PR_PAGE_IGNORE_INVALID;
			reason->page.do_ignore_starts_on_page = 1;
			fvwm_msg(
				WARN, "__place_window",
				"invalid style combination used: StartsOnPage"
				"/StartsOnDesk and SkipMapping don't work with"
				" ManualPlacement and TileManualPlacement."
				" Putting window on current page, please use"
				" another placement style or"
				" ActivePlacementHonorsStartsOnPage.");
		}
	}
	/* get the screen coordinates to place window on */
	if (SUSE_START_ON_SCREEN(&pstyle->flags))
	{
		if (flags.do_honor_starts_on_screen)
		{
			fscreen_scr_arg	 arg;
			arg.mouse_ev = NULL;
			arg.name = SGET_START_SCREEN(*pstyle);

			FScreenGetScrRect(&arg, FSCREEN_BY_NAME,
				&screen_g.x, &screen_g.y,
				&screen_g.width, &screen_g.height);
			fprintf(stderr, "MONITOR:  I SHOULD HAVE PLACED ON: '%s'\n",
				arg.name);
		}
		else
		{
			/* use global screen */
			FScreenGetScrRect(NULL, FSCREEN_GLOBAL,
				&screen_g.x, &screen_g.y,
				&screen_g.width, &screen_g.height);
			reason->screen.screen = "global";
		}
	}
	else
	{
		/* use current screen */
		FScreenGetScrRect(NULL, FSCREEN_CURRENT,
			&screen_g.x, &screen_g.y,
			&screen_g.width, &screen_g.height);
		reason->screen.screen = "current";
	}

	if (SPLACEMENT_MODE(&pstyle->flags) != PLACE_MINOVERLAPPERCENT &&
	    SPLACEMENT_MODE(&pstyle->flags) != PLACE_MINOVERLAP &&
	    SPLACEMENT_MODE(&pstyle->flags) != PLACE_POSITION)
	{
		/* TA:  In the case of PositionPlacement, the "ewmhiwa" option
		 * will have already modified this for us -- so don't do it
		 * for this placement policy.
		 */
		EWMH_GetWorkAreaIntersection(
			fw, &screen_g.x, &screen_g.y, &screen_g.width,
			&screen_g.height,
			SEWMH_PLACEMENT_MODE(&pstyle->flags));
		reason->screen.was_modified_by_ewmh_workingarea = 1;
	}
	reason->screen.g = screen_g;
	/* Don't alter the existing desk location during Capture/Recapture.  */
	if (!win_opts->flags.do_override_ppos)
	{
		fw->Desk = Scr.CurrentDesk;
		reason->desk.reason = PR_DESK_CURRENT;
	}
	else
	{
		reason->desk.reason = PR_DESK_CAPTURE;
	}
	if (S_IS_STICKY_ACROSS_DESKS(SFC(pstyle->flags)))
	{
		fw->Desk = Scr.CurrentDesk;
		reason->desk.reason = PR_DESK_STICKY;
	}
	else if (SUSE_START_ON_DESK(&pstyle->flags) && start_style.desk &&
		 flags.do_honor_starts_on_page)
	{
		fw->Desk = (start_style.desk > -1) ?
			start_style.desk - 1 : start_style.desk;
		reason->desk.reason = reason->desk.sod_reason;
	}
	else
	{
		if ((DO_USE_WINDOW_GROUP_HINT(fw)) &&
		    (fw->wmhints) && (fw->wmhints->flags & WindowGroupHint)&&
		    (fw->wmhints->window_group != None) &&
		    (fw->wmhints->window_group != Scr.Root))
		{
			/* Try to find the group leader or another window in
			 * the group */
			for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
			{
				if (FW_W(t) == fw->wmhints->window_group)
				{
					/* found the group leader, break out */
					fw->Desk = t->Desk;
					reason->desk.reason =
						PR_DESK_WINDOW_GROUP_LEADER;
					break;
				}
				else if (t->wmhints &&
					 (t->wmhints->flags &
					  WindowGroupHint) &&
					 (t->wmhints->window_group ==
					  fw->wmhints->window_group))
				{
					/* found a window from the same group,
					 * but keep looking for the group
					 * leader */
					fw->Desk = t->Desk;
					reason->desk.reason =
						PR_DESK_WINDOW_GROUP_MEMBER;
				}
			}
		}
		if ((IS_TRANSIENT(fw))&&(FW_W_TRANSIENTFOR(fw)!=None)&&
		    (FW_W_TRANSIENTFOR(fw) != Scr.Root))
		{
			/* Try to find the parent's desktop */
			for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
			{
				if (FW_W(t) == FW_W_TRANSIENTFOR(fw))
				{
					fw->Desk = t->Desk;
					reason->desk.reason =
						PR_DESK_TRANSIENT;
					break;
				}
			}
		}

		{
			/* migo - I am not sure this block is ever needed */

			Atom atype;
			int aformat;
			unsigned long nitems, bytes_remain;
			unsigned char *prop;

			if (
				XGetWindowProperty(
					dpy, FW_W(fw), _XA_WM_DESKTOP, 0L, 1L,
					True, _XA_WM_DESKTOP, &atype, &aformat,
					&nitems, &bytes_remain, &prop) ==
				Success)
			{
				if (prop != NULL)
				{
					fw->Desk = *(unsigned long *)prop;
					XFree(prop);
					reason->desk.reason =
						PR_DESK_XPROP_XA_WM_DESKTOP;
				}
			}
		}
	}
	reason->desk.desk = fw->Desk;
	/* I think it would be good to switch to the selected desk
	 * whenever a new window pops up, except during initialization */
	/*  RBW - 11/02/1998  --  I dont. */
	if (!win_opts->flags.do_override_ppos && !DO_NOT_SHOW_ON_MAP(fw))
	{
		if (Scr.CurrentDesk != fw->Desk)
		{
			reason->desk.do_switch_desk = 1;
		}
		goto_desk(fw->Desk);
	}
	/* Don't move viewport if SkipMapping, or if recapturing the window,
	 * adjust the coordinates later. Otherwise, just switch to the target
	 * page - it's ever so much simpler. */
	if (S_IS_STICKY_ACROSS_PAGES(SFC(pstyle->flags)))
	{
		reason->page.reason = PR_PAGE_STICKY;
	}
	else if (SUSE_START_ON_DESK(&pstyle->flags))
	{
		if (start_style.page_x != 0 && start_style.page_y != 0)
		{
			px = start_style.page_x - 1;
			py = start_style.page_y - 1;
			reason->page.reason = PR_PAGE_STYLE;
			px *= Scr.MyDisplayWidth;
			py *= Scr.MyDisplayHeight;
			if (!win_opts->flags.do_override_ppos &&
			    !DO_NOT_SHOW_ON_MAP(fw))
			{
				MoveViewport(px,py,True);
				reason->page.do_switch_page = 1;
			}
			else if (flags.do_honor_starts_on_page)
			{
				/*  Save the delta from current page */
				pdeltax = Scr.Vx - px;
				pdeltay = Scr.Vy - py;
				reason->page.do_honor_starts_on_page = 1;
			}
		}
	}

	/* pick a location for the window. */
	__place_get_placement_flags(
		&flags, fw, pstyle, win_opts, mode, reason);
	if (flags.do_not_use_wm_placement)
	{
		rc = __place_get_nowm_pos(
			exc, pstyle, attr_g, flags, screen_g, start_style,
			mode, win_opts, reason, pdeltax, pdeltay);
	}
	else
	{
		rc = __place_get_wm_pos(
			exc, pstyle, attr_g, flags, screen_g, start_style,
			mode, win_opts, reason, pdeltax, pdeltay);
	}
	reason->pos.x = attr_g->x;
	reason->pos.y = attr_g->y;

	return rc;
}

static void __place_handle_x_resources(
	FvwmWindow *fw, window_style *pstyle, pl_reason_t *reason)
{
	int client_argc = 0;
	char **client_argv = NULL;
	XrmValue rm_value;
	/* Used to parse command line of clients for specific desk requests. */
	/* Todo: check for multiple desks. */
	XrmDatabase db = NULL;
	static XrmOptionDescRec table [] = {
		/* Want to accept "-workspace N" or -xrm "fvwm*desk:N" as
		 * options to specify the desktop. I have to include dummy
		 * options that are meaningless since Xrm seems to allow -w to
		 * match -workspace if there would be no ambiguity. */
		{"-workspacf", "*junk", XrmoptionSepArg, (caddr_t) NULL},
		{"-workspace", "*desk", XrmoptionSepArg, (caddr_t) NULL},
		{"-xrn", NULL, XrmoptionResArg, (caddr_t) NULL},
		{"-xrm", NULL, XrmoptionResArg, (caddr_t) NULL},
	};
	int t1 = -1, t2 = -1, t3 = -1, spargs = 0;

	/* Find out if the client requested a specific desk on the command
	 * line.
	 *  RBW - 11/20/1998 - allow a desk of -1 to work.  */
	if (XGetCommand(dpy, FW_W(fw), &client_argv, &client_argc) == 0)
	{
		return;
	}
	if (client_argc <= 0 || client_argv == NULL)
	{
		return;
	}
	/* Get global X resources */
	MergeXResources(dpy, &db, False);
	/* command line takes precedence over all */
	MergeCmdLineResources(
		&db, table, 4, client_argv[0], &client_argc, client_argv,
		True);
	/* parse the database values */
	if (GetResourceString(db, "desk", client_argv[0], &rm_value) &&
	    rm_value.size != 0)
	{
		SGET_START_DESK(*pstyle) = atoi(rm_value.addr);
		/*  RBW - 11/20/1998  */
		if (SGET_START_DESK(*pstyle) > -1)
		{
			SSET_START_DESK(
				*pstyle, SGET_START_DESK(*pstyle) + 1);
		}
		reason->desk.sod_reason = PR_DESK_X_RESOURCE_DESK;
		pstyle->flags.use_start_on_desk = 1;
	}
	if (GetResourceString(db, "fvwmscreen", client_argv[0], &rm_value) &&
	    rm_value.size != 0)
	{
		SSET_START_SCREEN(*pstyle, rm_value.addr);
		reason->screen.reason = PR_SCREEN_X_RESOURCE_FVWMSCREEN;
		reason->screen.screen = SGET_START_SCREEN(*pstyle);
		pstyle->flags.use_start_on_screen = 1;
	}
	if (GetResourceString(db, "page", client_argv[0], &rm_value) &&
	    rm_value.size != 0)
	{
		spargs = sscanf(
			rm_value.addr, "%d %d %d", &t1, &t2, &t3);
		switch (spargs)
		{
		case 1:
			pstyle->flags.use_start_on_desk = 1;
			SSET_START_DESK(*pstyle, (t1 > -1) ? t1 + 1 : t1);
			reason->desk.sod_reason = PR_DESK_X_RESOURCE_PAGE;
			break;
		case 2:
			pstyle->flags.use_start_on_desk = 1;
			SSET_START_PAGE_X(*pstyle, (t1 > -1) ? t1 + 1 : t1);
			SSET_START_PAGE_Y(*pstyle, (t2 > -1) ? t2 + 1 : t2);
			reason->page.reason = PR_PAGE_X_RESOURCE_PAGE;
			reason->page.px = SGET_START_PAGE_X(*pstyle);
			reason->page.py = SGET_START_PAGE_Y(*pstyle);
			break;
		case 3:
			pstyle->flags.use_start_on_desk = 1;
			SSET_START_DESK(*pstyle, (t1 > -1) ? t1 + 1 : t1);
			reason->desk.sod_reason =
				PR_DESK_X_RESOURCE_PAGE;
			SSET_START_PAGE_X(*pstyle, (t2 > -1) ? t2 + 1 : t2);
			SSET_START_PAGE_Y(*pstyle, (t3 > -1) ? t3 + 1 : t3);
			reason->page.reason = PR_PAGE_X_RESOURCE_PAGE;
			reason->page.px = SGET_START_PAGE_X(*pstyle);
			reason->page.py = SGET_START_PAGE_Y(*pstyle);
			break;
		default:
			break;
		}
	}
	XFreeStringList(client_argv);
	XrmDestroyDatabase(db);

	return;
}

static void __explain_placement(FvwmWindow *fw, pl_reason_t *reason)
{
	char explanation[2048];
	char *r;
	char *s;
	int do_show_page;
	int is_placed_by_algo;

	*explanation = 0;
	s = explanation;
	strcat(s, "placed new window 0x%x '%s':\n");
	s += strlen(s);
	sprintf(
		s, "  initial size %dx%d\n", fw->g.frame.width,
		fw->g.frame.height);
	s += strlen(s);
	switch (reason->desk.reason)
	{
	case PR_DESK_CURRENT:
		r = "current desk";
		break;
	case PR_DESK_STYLE:
		r = "specified by style";
		break;
	case PR_DESK_X_RESOURCE_DESK:
		r = "specified by 'desk' X resource";
		break;
	case PR_DESK_X_RESOURCE_PAGE:
		r = "specified by 'page' X resource";
		break;
	case PR_DESK_CAPTURE:
		r = "window was (re)captured";
		break;
	case PR_DESK_STICKY:
		r = "window is sticky";
		break;
	case PR_DESK_WINDOW_GROUP_LEADER:
		r = "same desk as window group leader";
		break;
	case PR_DESK_WINDOW_GROUP_MEMBER:
		r = "same desk as window group member";
		break;
	case PR_DESK_TRANSIENT:
		r = "transient window placed on same desk as parent";
		break;
	case PR_DESK_XPROP_XA_WM_DESKTOP:
		r = "specified by _XA_WM_DESKTOP property";
		break;
	default:
		r = "bug";
		break;
	}
	sprintf(s, "  desk %d (%s)\n", reason->desk.desk, r);
	s += strlen(s);
	if (reason->desk.do_switch_desk == 1)
	{
		sprintf(s, "    (switched to desk)\n");
		s += strlen(s);
	}
	/* page */
	do_show_page = 1;
	switch (reason->page.reason)
	{
	case PR_PAGE_CURRENT:
		do_show_page = 0;
		r = "current page";
		break;
	case PR_PAGE_STYLE:
		r = "specified by style";
		break;
	case PR_PAGE_X_RESOURCE_PAGE:
		r = "specified by 'page' X resource";
		break;
	case PR_PAGE_IGNORE_CAPTURE:
		r = "window was (re)captured";
		break;
	case PR_PAGE_IGNORE_INVALID:
		r = "requested page ignored because of invalid style"
			" combination";
		break;
	case PR_PAGE_STICKY:
		do_show_page = 0;
		r = "current page (window is sticky)";
		break;
	default:
		r = "bug";
		break;
	}
	if (do_show_page == 0)
	{
		sprintf(s, "  %s\n", r);
	}
	else
	{
		sprintf(
			s, "  page %d %d (%s)\n", reason->page.px - 1,
			reason->page.py - 1, r);
	}
	s += strlen(s);
	if (reason->page.do_switch_page == 1)
	{
		sprintf(s, "    (switched to page)\n");
		s += strlen(s);
	}
	if (reason->page.do_ignore_starts_on_page == 1)
	{
		sprintf(s, "    (possibly ignored StartsOnPage)\n");
		s += strlen(s);
	}
	/* screen */
	if (FScreenIsEnabled() == True)
	{
		switch (reason->screen.reason)
		{
		case PR_SCREEN_CURRENT:
			r = "current screen";
			break;
		case PR_SCREEN_STYLE:
			r = "specified by style";
			break;
		case PR_SCREEN_X_RESOURCE_FVWMSCREEN:
			r = "specified by 'fvwmscreen' X resource";
			break;
		case PR_SCREEN_IGNORE_CAPTURE:
			r = "window was (re)captured";
			break;
		default:
			r = "bug";
			break;
		}
		sprintf(
			s, "  screen: %s: %d %d %dx%d (%s)\n",
			reason->screen.screen, reason->screen.g.x,
			reason->screen.g.y, reason->screen.g.width,
			reason->screen.g.height, r);
		s += strlen(s);
		if (reason->screen.was_modified_by_ewmh_workingarea == 1)
		{
			sprintf(
				s, "    (screen area modified by EWMH working"
				" area)\n");
			s += strlen(s);
		}
	}
	/* position */
	is_placed_by_algo = 0;
	switch (reason->pos.reason)
	{
	case PR_POS_NORMAL:
		is_placed_by_algo = 1;
		r = "normal placement";
		break;
	case PR_POS_IGNORE_PPOS:
		is_placed_by_algo = 1;
		r = "ignored program specified position";
		break;
	case PR_POS_USE_PPOS:
		r = "used program specified position";
		break;
	case PR_POS_IGNORE_USPOS:
		is_placed_by_algo = 1;
		r = "ignored user specified position";
		break;
	case PR_POS_USE_USPOS:
		r = "used user specified position";
		break;
	case PR_POS_PLACE_AGAIN:
		is_placed_by_algo = 1;
		r = "by PlaceAgain command";
		break;
	case PR_POS_CAPTURE:
		r = "window was (re)captured";
		break;
	case PR_POS_USPOS_OVERRIDE_SOS:
		r = "StartsOnPage style overridden by application via USPos";
		break;
	default:
		r = "bug";
		break;
	}
	sprintf(s, "  position %d %d", reason->pos.x, reason->pos.y);
	s += strlen(s);
	if (is_placed_by_algo == 1)
	{
		char *a;
		char *b;

		b = "";
		switch (reason->pos.algo)
		{
		case PLACE_POSITION:
			a = "Position args: ";
			b = reason->pos.pl_position_string;
			break;
		case PLACE_TILEMANUAL:
			a = "TileManual";
			break;
		case PLACE_MANUAL:
		case PLACE_MANUAL_B:
			a = "Manual";
			break;
		case PLACE_MINOVERLAPPERCENT:
			a = "MinOverlapPercent";
			break;
		case PLACE_TILECASCADE:
			a = "TileCascade";
			break;
		case PLACE_CASCADE:
		case PLACE_CASCADE_B:
			a = "Cascade";
			break;
		case PLACE_MINOVERLAP:
			a = "MinOverlap";
			break;
		default:
			a = "bug";
			break;
		}
		sprintf(s, ", placed by fvwm (%s)\n", r);
		s += strlen(s);
		sprintf(s, "    placement method: %s%s\n", a, b);
		s += strlen(s);
		if (reason->pos.do_not_manual_icon_placement == 1)
		{
			sprintf(s, "    (icon not placed manually)\n");
			s += strlen(s);
		}
		if (reason->pos.is_pl_position_string_invalid == 1)
		{
			sprintf(s, "    (invalid position string)\n");
			s += strlen(s);
		}
		if (reason->pos.has_tile_failed == 1)
		{
			sprintf(s, "    (tile placement failed)\n");
			s += strlen(s);
		}
		if (reason->pos.has_manual_failed == 1)
		{
			sprintf(s, "    (manual placement failed)\n");
			s += strlen(s);
		}
		if (reason->pos.has_placement_failed == 1)
		{
			sprintf(s, "    (placement failed default pos 0 0)\n");
			s += strlen(s);
		}
	}
	else
	{
		sprintf(s, "  (%s)\n", r);
		s += strlen(s);
	}
	if (reason->pos.do_adjust_off_screen == 1)
	{
		sprintf(s, "    (adjusted to force window on screen)\n");
		s += strlen(s);
	}
	if (reason->pos.do_adjust_off_page == 1)
	{
		sprintf(s, "    (adjusted to force window on page)\n");
		s += strlen(s);
	}
	fvwm_msg(
		INFO, "__explain_placement", explanation, (int)FW_W(fw),
		fw->name.name);

	return;
}

/* ---------------------------- interface functions ------------------------ */

Bool setup_window_placement(
	FvwmWindow *fw, window_style *pstyle, rectangle *attr_g,
	initial_window_options_t *win_opts, placement_mode_t mode)
{
	int rc;
	const exec_context_t *exc;
	exec_context_changes_t ecc;
	pl_reason_t reason;
	pl_start_style_t start_style;

	memset(&reason, 0, sizeof(reason));
	if (pstyle->flags.use_start_on_desk)
	{
		reason.desk.sod_reason = PR_DESK_STYLE;
		reason.page.px = SGET_START_PAGE_X(*pstyle);
		reason.page.py = SGET_START_PAGE_Y(*pstyle);
	}
	if (pstyle->flags.use_start_on_screen)
	{
		reason.screen.reason = PR_SCREEN_STYLE;
		reason.screen.screen = SGET_START_SCREEN(*pstyle);
	}
	__place_handle_x_resources(fw, pstyle, &reason);
	if (pstyle->flags.do_start_iconic)
	{
		win_opts->initial_state = IconicState;
	}
	ecc.type = EXCT_NULL;
	ecc.w.fw = fw;
	exc = exc_create_context(&ecc, ECC_TYPE | ECC_FW);
	start_style.desk = SGET_START_DESK(*pstyle);
	start_style.page_x = SGET_START_PAGE_X(*pstyle);
	start_style.page_y = SGET_START_PAGE_Y(*pstyle);
	start_style.screen = SGET_START_SCREEN(*pstyle);
	rc = __place_window(
		exc, pstyle, attr_g, start_style, mode, win_opts, &reason);
	exc_destroy_context(exc);
	if (Scr.bo.do_explain_window_placement == 1)
	{
		__explain_placement(fw, &reason);
	}

	return (rc == 0) ? False : True;
}

/* ---------------------------- builtin commands --------------------------- */

void CMD_PlaceAgain(F_CMD_ARGS)
{
	int old_desk;
	char *token;
	float noMovement[1] = {1.0};
	float *ppctMovement = noMovement;
	rectangle attr_g;
	XWindowAttributes attr;
	Bool do_move_animated = False;
	Bool do_place_icon = False;
	FvwmWindow * const fw = exc->w.fw;

	if (!XGetWindowAttributes(dpy, FW_W(fw), &attr))
	{
		return;
	}
	while ((token = PeekToken(action, &action)) != NULL)
	{
		if (StrEquals("Anim", token))
		{
			ppctMovement = NULL;
			do_move_animated = True;
		}
		else if (StrEquals("icon", token))
		{
			do_place_icon = True;
		}
	}
	old_desk = fw->Desk;
	if (IS_ICONIFIED(fw) && !do_place_icon)
	{
		return;
	}
	if (IS_ICONIFIED(fw) && do_place_icon)
	{
		rectangle new_g;
		rectangle old_g;

		if (IS_ICON_SUPPRESSED(fw))
		{
			return;
		}
		fw->Desk = Scr.CurrentDesk;
		get_icon_geometry(fw, &old_g);
		SET_ICON_MOVED(fw, 0);
		AutoPlaceIcon(fw, NULL, False);
		get_icon_geometry(fw, &new_g);
		__move_icon(
			fw, new_g.x, new_g.y, old_g.x, old_g.y,
			do_move_animated, False);
	}
	else
	{
		window_style style;
		initial_window_options_t win_opts;

		memset(&win_opts, 0, sizeof(win_opts));
		lookup_style(fw, &style);
		attr_g.x = attr.x;
		attr_g.y = attr.y;
		attr_g.width = attr.width;
		attr_g.height = attr.height;

		setup_window_placement(
			exc->w.fw, &style, &attr_g, &win_opts, PLACE_AGAIN);
		AnimatedMoveFvwmWindow(
			fw, FW_W_FRAME(fw), -1, -1, attr_g.x, attr_g.y, False,
			-1, ppctMovement);
	}
	if (fw->Desk != old_desk)
	{
		int new_desk = fw->Desk;

		fw->Desk = old_desk;
		do_move_window_to_desk(fw, new_desk);
	}

	return;
}
