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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This module is all new
 * by Rob Nation
 *
 * This code does smart-placement initial window placement stuff
 *
 * Copyright 1994 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved . No guarantees or
 * warrantees of any sort whatsoever are given or implied or anything.
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
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

/* ---------------------------- local macros ------------------------------- */

#ifndef MIN
#define MIN(A,B) ((A)<(B)? (A):(B))
#endif
#ifndef MAX
#define MAX(A,B) ((A)>(B)? (A):(B))
#endif

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
		unsigned do_not_manual_icon_placement : 1;
		unsigned do_adjust_off_screen : 1;
		unsigned do_adjust_off_page : 1;
		unsigned has_tile_failed : 1;
		unsigned has_manual_failed : 1;
	} pos;
	struct
	{
		preason_screen_t reason;
		int screen;
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
} placement_reason_t;

typedef struct
{
	int desk;
	int page_x;
	int page_y;
	int screen;
} placement_start_style_t;

typedef struct
{
	unsigned do_forbid_manual_placement : 1;
	unsigned do_honor_starts_on_page : 1;
	unsigned do_honor_starts_on_screen : 1;
	unsigned is_smartly_placed : 1;
	unsigned do_not_use_wm_placement : 1;
} placement_flags_t;

/* ---------------------------- forward declarations ----------------------- */

static int get_next_x(
	FvwmWindow *t, rectangle *screen_g, int x, int y, int pdeltax,
	int pdeltay, int use_percent);
static int get_next_y(
	FvwmWindow *t, rectangle *screen_g, int y, int pdeltay,
	int use_percent);
static float test_fit(
	FvwmWindow *t, style_flags *sflags, rectangle *screen_g,
	int x11, int y11, float aoimin, int pdeltax, int pdeltay,
	int use_percent);
static void CleverPlacement(
	FvwmWindow *t, style_flags *sflags, rectangle *screen_g,
	int *x, int *y, int pdeltax, int pdeltay, int use_percent);

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

static int SmartPlacement(
	FvwmWindow *t, rectangle *screen_g,
	int width, int height, int *x, int *y, int pdeltax, int pdeltay)
{
	int PageLeft   = screen_g->x - pdeltax;
	int PageTop    = screen_g->y - pdeltay;
	int PageRight  = PageLeft + screen_g->width;
	int PageBottom = PageTop + screen_g->height;
	int temp_h;
	int temp_w;
	int test_x = 0;
	int test_y = 0;
	int loc_ok = False;
	int tw = 0;
	int tx = 0;
	int ty = 0;
	int th = 0;
	FvwmWindow *test_fw;
	int stickyx;
	int stickyy;
	rectangle g;
	Bool rc;

	test_x = PageLeft;
	test_y = PageTop;
	temp_h = height;
	temp_w = width;

	for ( ; test_y + temp_h < PageBottom && !loc_ok; )
	{
		test_x = PageLeft;
		while (test_x + temp_w < PageRight && !loc_ok)
		{
			loc_ok = True;
			test_fw = Scr.FvwmRoot.next;
			for ( ; test_fw != NULL && loc_ok;
			      test_fw = test_fw->next)
			{
				if (t == test_fw ||
				    IS_EWMH_DESKTOP(FW_W(test_fw)))
				{
					continue;
				}
				/*  RBW - account for sticky windows...  */
				if (test_fw->Desk == t->Desk ||
				    IS_STICKY_ACROSS_DESKS(test_fw))
				{
					if (IS_STICKY_ACROSS_PAGES(test_fw))
					{
						stickyx = pdeltax;
						stickyy = pdeltay;
					}
					else
					{
						stickyx = 0;
						stickyy = 0;
					}
					rc = get_visible_window_or_icon_geometry(
						test_fw, &g);
					if (rc == True &&
					    (PLACEMENT_AVOID_ICON == 0 ||
					     !IS_ICONIFIED(test_fw)))
					{
						tx = g.x - stickyx;
						ty = g.y - stickyy;
						tw = g.width;
						th = g.height;
						if (tx < test_x + width  &&
						    test_x < tx + tw &&
						    ty < test_y + height &&
						    test_y < ty + th)
						{
							/* window overlaps,
							 * look for a
							 * different place */
							loc_ok = False;
							test_x = tx + tw - 1;
						}
					}
				}
			} /* for */
			if (!loc_ok)
			{
				test_x +=1;
			}
		} /* while */
		if (!loc_ok)
		{
			test_y +=1;
		}
	} /* while */
	if (loc_ok == False)
	{
		return False;
	}
	*x = test_x;
	*y = test_y;

	return True;
}


/* CleverPlacement by Anthony Martin <amartin@engr.csulb.edu>
 * This function will place a new window such that there is a minimum amount
 * of interference with other windows.  If it can place a window without any
 * interference, fine.  Otherwise, it places it so that the area of of
 * interference between the new window and the other windows is minimized */
static void CleverPlacement(
	FvwmWindow *t, style_flags *sflags, rectangle *screen_g,
	int *x, int *y, int pdeltax, int pdeltay, int use_percent)
{
	int test_x;
	int test_y;
	int xbest;
	int ybest;
	/* area of interference */
	float aoi;
	float aoimin;
	int PageLeft = screen_g->x - pdeltax;
	int PageTop = screen_g->y - pdeltay;

	test_x = PageLeft;
	test_y = PageTop;
	aoi = test_fit(
		t, sflags, screen_g, test_x, test_y, -1, pdeltax, pdeltay,
		use_percent);
	aoimin = aoi;
	xbest = test_x;
	ybest = test_y;

	while (aoi != 0 && aoi != -1)
	{
		if (aoi > 0)
		{
			/* Windows interfere.  Try next x. */
			test_x = get_next_x(
				t, screen_g, test_x, test_y, pdeltax, pdeltay,
				use_percent);
		}
		else
		{
			/* Out of room in x direction. Try next y. Reset x.*/
			test_x = PageLeft;
			test_y = get_next_y(
				t, screen_g, test_y, pdeltay, use_percent);
		}
		aoi = test_fit(
			t, sflags, screen_g, test_x,test_y, aoimin, pdeltax,
			pdeltay, use_percent);
		/* I've added +0.0001 because whith my machine the < test fail
		 * with certain *equal* float numbers! */
		if (aoi >= 0 && aoi + 0.0001 < aoimin)
		{
			xbest = test_x;
			ybest = test_y;
			aoimin = aoi;
		}
	}
	*x = xbest;
	*y = ybest;

	return;
}

#define GET_NEXT_STEP 5
static int get_next_x(
	FvwmWindow *t, rectangle *screen_g, int x, int y, int pdeltax,
	int pdeltay, int use_percent)
{
	FvwmWindow *testw;
	int xnew;
	int xtest;
	int PageLeft = screen_g->x - pdeltax;
	int PageRight = PageLeft + screen_g->width;
	int stickyx;
	int stickyy;
	int start,i;
	int win_left;
	rectangle g;
	Bool rc;

	if (use_percent)
	{
		start = 0;
	}
	else
	{
		start = GET_NEXT_STEP;
	}

	/* Test window at far right of screen */
	xnew = PageRight;
	xtest = PageRight - t->frame_g.width;
	if (xtest > x)
	{
		xnew = MIN(xnew, xtest);
	}
	/* test the borders of the working area */
	xtest = PageLeft + Scr.Desktops->ewmh_working_area.x;
	if (xtest > x)
	{
		xnew = MIN(xnew, xtest);
	}
	xtest = PageLeft +
		(Scr.Desktops->ewmh_working_area.x +
		 Scr.Desktops->ewmh_working_area.width) - t->frame_g.width;
	if (xtest > x)
	{
		xnew = MIN(xnew, xtest);
	}
	/* Test the values of the right edges of every window */
	for (testw = Scr.FvwmRoot.next ; testw != NULL ; testw = testw->next)
	{
		if (testw == t ||
		    (testw->Desk != t->Desk &&
		     !IS_STICKY_ACROSS_DESKS(testw)) ||
		    IS_EWMH_DESKTOP(FW_W(testw)))
		{
			continue;
		}
		if (IS_STICKY_ACROSS_PAGES(testw))
		{
			stickyx = pdeltax;
			stickyy = pdeltay;
		}
		else
		{
			stickyx = 0;
			stickyy = 0;
		}
		if (IS_ICONIFIED(testw))
		{
			rc = get_visible_icon_geometry(testw, &g);
			if (rc == True && y < g.y + g.height - stickyy &&
			    g.y - stickyy < t->frame_g.height + y)
			{
				win_left = PageLeft + g.x - stickyx -
					t->frame_g.width;
				for (i = start; i <= GET_NEXT_STEP; i++)
				{
					xtest = win_left + g.width *
						(GET_NEXT_STEP - i) /
						GET_NEXT_STEP;
					if (xtest > x)
					{
						xnew = MIN(xnew, xtest);
					}
				}
				win_left = PageLeft + g.x - stickyx;
				for(i=start; i <= GET_NEXT_STEP; i++)
				{
					xtest = (win_left) + g.width * i /
						GET_NEXT_STEP;
					if (xtest > x)
					{
						xnew = MIN(xnew, xtest);
					}
				}
			}
		}
		else if (
			y <
			testw->frame_g.height + testw->frame_g.y - stickyy &&
			testw->frame_g.y - stickyy < t->frame_g.height + y &&
			PageLeft <
			testw->frame_g.width + testw->frame_g.x - stickyx &&
			testw->frame_g.x - stickyx < PageRight)
		{
			win_left = PageLeft + pdeltax + testw->frame_g.x -
				stickyx - t->frame_g.width;
			for(i=start; i <= GET_NEXT_STEP; i++)
			{
				xtest = (win_left) + (testw->frame_g.width) *
					(GET_NEXT_STEP - i)/GET_NEXT_STEP;
				if (xtest > x)
				{
					xnew = MIN(xnew, xtest);
				}
			}
			win_left = PageLeft + pdeltax + testw->frame_g.x -
				stickyx;
			for(i=start; i <= GET_NEXT_STEP; i++)
			{
				xtest = (win_left) + (testw->frame_g.width) *
					i/GET_NEXT_STEP;
				if (xtest > x)
				{
					xnew = MIN(xnew, xtest);
				}
			}
		}
	}

	return xnew;
}

static int get_next_y(
	FvwmWindow *t, rectangle *screen_g, int y, int pdeltay,
	int use_percent)
{
	FvwmWindow *testw;
	int ynew;
	int ytest;
	int PageBottom = screen_g->y + screen_g->height - pdeltay;
	int stickyy;
	int win_top;
	int start;
	int i;
	rectangle g;

	if (use_percent)
	{
		start = 0;
	}
	else
	{
		start = GET_NEXT_STEP;
	}

	/* Test window at far bottom of screen */
	ynew = PageBottom;
	ytest = PageBottom - t->frame_g.height;
	if (ytest > y)
	{
		ynew = MIN(ynew, ytest);
	}
	/* test the borders of the working area */
	ytest = screen_g->y + Scr.Desktops->ewmh_working_area.y - pdeltay;
	if (ytest > y)
	{
		ynew = MIN(ynew, ytest);
	}
	ytest = screen_g->y +
		(Scr.Desktops->ewmh_working_area.y +
		 Scr.Desktops->ewmh_working_area.height) - t->frame_g.height;
	if (ytest > y)
		ynew = MIN(ynew, ytest);
	/* Test the values of the bottom edge of every window */
	for (testw = Scr.FvwmRoot.next ; testw != NULL ; testw = testw->next)
	{
		if (testw == t ||
		    (testw->Desk != t->Desk && !IS_STICKY_ACROSS_DESKS(testw))
		    || IS_EWMH_DESKTOP(FW_W(testw)))
		{
			continue;
		}

		if (IS_STICKY_ACROSS_PAGES(testw))
		{
			stickyy = pdeltay;
		}
		else
		{
			stickyy = 0;
		}

		if (IS_ICONIFIED(testw))
		{
			get_visible_icon_geometry(testw, &g);
			win_top = g.y - stickyy;
			for(i=start; i <= GET_NEXT_STEP; i++)
			{
				ytest = win_top + g.height * i / GET_NEXT_STEP;
				if (ytest > y)
				{
					ynew = MIN(ynew, ytest);
				}
			}
			win_top = g.y - stickyy - t->frame_g.height;
			for(i=start; i <= GET_NEXT_STEP; i++)
			{
				ytest = win_top + g.height *
					(GET_NEXT_STEP - i) / GET_NEXT_STEP;
				if (ytest > y)
				{
					ynew = MIN(ynew, ytest);
				}
			}
		}
		else
		{
			win_top = testw->frame_g.y - stickyy;;
			for(i=start; i <= GET_NEXT_STEP; i++)
			{
				ytest = (win_top) + (testw->frame_g.height) *
					i/GET_NEXT_STEP;
				if (ytest > y)
				{
					ynew = MIN(ynew, ytest);
				}
			}
			win_top = testw->frame_g.y - stickyy -
				t->frame_g.height;
			for(i=start; i <= GET_NEXT_STEP; i++)
			{
				ytest = win_top + (testw->frame_g.height) *
					(GET_NEXT_STEP - i)/GET_NEXT_STEP;
				if (ytest > y)
				{
					ynew = MIN(ynew, ytest);
				}
			}
		}
	}

	return ynew;
}

static float test_fit(
	FvwmWindow *t, style_flags *sflags, rectangle *screen_g,
	int x11, int y11, float aoimin, int pdeltax, int pdeltay,
	int use_percent)
{
	FvwmWindow *testw;
	int x12;
	int x21;
	int x22;
	int y12;
	int y21;
	int y22;
	/* xleft, xright, ytop, ybottom */
	int xl;
	int xr;
	int yt;
	int yb;
	/* area of interference */
	float aoi = 0;
	float anew;
	float cover_factor = 0;
	float avoidance_factor;
	int PageRight  = screen_g->x + screen_g->width - pdeltax;
	int PageBottom = screen_g->y + screen_g->height - pdeltay;
	int stickyx, stickyy;
	rectangle g;
	Bool rc;

	x12 = x11 + t->frame_g.width;
	y12 = y11 + t->frame_g.height;

	if (y12 > PageBottom) /* No room in y direction */
	{
		return -1;
	}
	if (x12 > PageRight) /* No room in x direction */
	{
		return -2;
	}
	for (testw = Scr.FvwmRoot.next ; testw != NULL ; testw = testw->next)
	{
		if (testw == t ||
		    (testw->Desk != t->Desk && !IS_STICKY_ACROSS_DESKS(testw))
		    || IS_EWMH_DESKTOP(FW_W(testw)))
		{
			continue;
		}

		if (IS_STICKY_ACROSS_PAGES(testw))
		{
			stickyx = pdeltax;
			stickyy = pdeltay;
		}
		else
		{
			stickyx = 0;
			stickyy = 0;
		}

		rc = get_visible_window_or_icon_geometry(testw, &g);
		x21 = g.x - stickyx;
		y21 = g.y - stickyy;
		x22 = x21 + g.width;
		y22 = y21 + g.height;
		if (x11 < x22 && x12 > x21 &&
		    y11 < y22 && y12 > y21)
		{
			/* Windows interfere */
			xl = MAX(x11, x21);
			xr = MIN(x12, x22);
			yt = MAX(y11, y21);
			yb = MIN(y12, y22);
			anew = (xr - xl) * (yb - yt);
			if (IS_ICONIFIED(testw))
			{
				avoidance_factor = ICON_PLACEMENT_PENALTY(
					testw);
			}
			else if(compare_window_layers(testw, t) > 0)
			{
				avoidance_factor = ONTOP_PLACEMENT_PENALTY(
					testw);
			}
			else if(compare_window_layers(testw, t) < 0)
			{
				avoidance_factor = BELOW_PLACEMENT_PENALTY(
					testw);
			}
			else if(IS_STICKY_ACROSS_PAGES(testw) ||
				IS_STICKY_ACROSS_DESKS(testw))
			{
				avoidance_factor = STICKY_PLACEMENT_PENALTY(
					testw);
			}
			else
			{
				avoidance_factor = NORMAL_PLACEMENT_PENALTY(
					testw);
			}

			if (use_percent)
			{
				cover_factor = 0;
				if ((x22 - x21) * (y22 - y21) != 0 &&
				    (x12 - x11) * (y12 - y11) != 0)
				{
					anew = 100 * MAX(
						anew / ((x22 - x21) *
							(y22 - y21)),
						anew / ((x12 - x11) *
							(y12 - y11)));
					if (anew >= 99)
					{
						cover_factor =
							PERCENTAGE_99_PENALTY(
								testw);
					}
					else if (anew > 94)
					{
						cover_factor =
							PERCENTAGE_95_PENALTY(
								testw);
					}
					else if (anew > 84)
					{
						cover_factor =
							PERCENTAGE_85_PENALTY(
								testw);
					}
					else if (anew > 74)
					{
						cover_factor =
							PERCENTAGE_75_PENALTY(
								testw);
					}
				}
				avoidance_factor += (avoidance_factor >= 1) ?
					cover_factor : 0;
			}

			if (SEWMH_PLACEMENT_MODE(sflags) ==
			    EWMH_USE_DYNAMIC_WORKING_AREA &&
			    !DO_EWMH_IGNORE_STRUT_HINTS(testw) &&
			    (testw->dyn_strut.left > 0 ||
			     testw->dyn_strut.right > 0 ||
			     testw->dyn_strut.top > 0 ||
			     testw->dyn_strut.bottom > 0))
			{
				/* if we intersect a window which reserves
				 * space */
				avoidance_factor += (avoidance_factor >= 1)?
					EWMH_STRUT_PLACEMENT_PENALTY(t) : 0;
			}

			anew *= avoidance_factor;
			aoi += anew;
			if (aoi > aoimin && aoimin != -1)
			{
				return aoi;
			}
		}
	}
	/* now handle the working area */
	if (SEWMH_PLACEMENT_MODE(sflags) == EWMH_USE_WORKING_AREA)
	{
		aoi += EWMH_STRUT_PLACEMENT_PENALTY(t) *
			EWMH_GetStrutIntersection(
				x11, y11, x12, y12, use_percent);
	}
	else /* EWMH_USE_DYNAMIC_WORKING_AREA, count the base strut */
	{
		aoi +=  EWMH_STRUT_PLACEMENT_PENALTY(t) *
			EWMH_GetBaseStrutIntersection(
				x11, y11, x12, y12, use_percent);
	}
	return aoi;
}

static void __place_get_placement_flags(
	placement_flags_t *ret_flags, FvwmWindow *fw, style_flags *sflags,
	initial_window_options_t *win_opts, int mode,
	placement_reason_t *reason)
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
		override_ppos = SUSE_NO_TRANSIENT_PPOSITION(sflags);
		override_uspos = SUSE_NO_TRANSIENT_USPOSITION(sflags);
	}
	else
	{
		override_ppos = SUSE_NO_PPOSITION(sflags);
		override_uspos = SUSE_NO_USPOSITION(sflags);
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
			reason->pos.reason = PR_POS_USE_USPOS;
		}
	}
	if (mode == PLACE_AGAIN)
	{
		ret_flags->do_not_use_wm_placement = False;
		reason->pos.reason = PR_POS_PLACE_AGAIN;
	}
	else if (has_ppos || has_uspos)
	{
		ret_flags->do_not_use_wm_placement = True;
	}
	else if (win_opts->flags.do_override_ppos)
	{
		ret_flags->do_not_use_wm_placement = True;
		reason->pos.reason = PR_POS_CAPTURE;
	}
	else if (!ret_flags->do_honor_starts_on_page &&
		 fw->wmhints && (fw->wmhints->flags & StateHint) &&
		 fw->wmhints->initial_state == IconicState)
	{
		ret_flags->do_forbid_manual_placement = True;
		reason->pos.do_not_manual_icon_placement = 1;
	}

	return;
}

static Bool __place_get_wm_pos(
	const exec_context_t *exc, style_flags *sflags, rectangle *attr_g,
	placement_flags_t flags, rectangle screen_g,
	placement_start_style_t start_style, int mode,
	initial_window_options_t *win_opts, placement_reason_t *reason,
	int pdeltax, int pdeltay)
{
	unsigned int placement_mode = SPLACEMENT_MODE(sflags);
	FvwmWindow *fw = exc->w.fw;
	Bool rc;
	int DragWidth;
	int DragHeight;
	size_borders b;
	int PageLeft;
	int PageTop;
	int PageRight;
	int PageBottom;
	int xl;
	int yt;

	rc = False;
	PageLeft   = screen_g.x - pdeltax;
	PageTop    = screen_g.y - pdeltay;
	PageRight  = PageLeft + screen_g.width;
	PageBottom = PageTop + screen_g.height;
	xl = -1;
	yt = PageTop;
	/* override if Manual placement happen */
	SET_PLACED_BY_FVWM(fw, True);
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
	/* first, try various "smart" placement */
	reason->pos.algo = placement_mode;
	switch (placement_mode)
	{
	case PLACE_CENTER:
		attr_g->x = (screen_g.width - fw->frame_g.width) / 2;
		attr_g->y = ((screen_g.height - fw->frame_g.height) / 2);
		/* Don't let the upper left corner be offscreen. */
		if (attr_g->x < PageLeft)
		{
			attr_g->x = PageLeft;
		}
		if (attr_g->y < PageTop)
		{
			attr_g->y = PageTop;
		}
		break;
	case PLACE_TILEMANUAL:
		flags.is_smartly_placed = SmartPlacement(
			fw, &screen_g, fw->frame_g.width, fw->frame_g.height,
			&xl, &yt, pdeltax, pdeltay);
		if (flags.is_smartly_placed)
		{
			break;
		}
		reason->pos.has_tile_failed = 1;
		/* fall through to manual placement */
	case PLACE_MANUAL:
	case PLACE_MANUAL_B:
		/* either "smart" placement fail and the second
		 * choice is a manual placement (TileManual) or we
		 * have a manual placement in any case (Manual) */
		xl = 0;
		yt = 0;
		if (GrabEm(CRS_POSITION, GRAB_NORMAL))
		{
			int mx;
			int my;

			/* Grabbed the pointer - continue */
			MyXGrabServer(dpy);
			if (XGetGeometry(
				    dpy, FW_W(fw), &JunkRoot, &JunkX, &JunkY,
				    (unsigned int *)&DragWidth,
				    (unsigned int *)&DragHeight, &JunkBW,
				    &JunkDepth) == 0)
			{
				MyXUngrabServer(dpy);
				UngrabEm(GRAB_NORMAL);
				return False;
			}
			SET_PLACED_BY_FVWM(fw,False);
			MyXGrabKeyboard(dpy);
			DragWidth = fw->frame_g.width;
			DragHeight = fw->frame_g.height;

			if (Scr.SizeWindow != None)
			{
				XMapRaised(dpy, Scr.SizeWindow);
			}
			FScreenGetScrRect(
				NULL, FSCREEN_GLOBAL, &mx, &my, NULL, NULL);
			if (__move_loop(
				    exc, mx, my, DragWidth, DragHeight, &xl,
				    &yt, False))
			{
				/* resize too */
				rc = True;
			}
			else
			{
				rc = False;
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
			xl = 0;
			yt = 0;
			reason->pos.has_manual_failed = 1;
		}
		if (flags.do_honor_starts_on_page)
		{
			xl -= pdeltax;
			yt -= pdeltay;
		}
		attr_g->y = yt;
		attr_g->x = xl;
		break;
	case PLACE_MINOVERLAPPERCENT:
		CleverPlacement(
			fw, sflags, &screen_g, &xl, &yt, pdeltax, pdeltay, 1);
		flags.is_smartly_placed = True;
		break;
	case PLACE_TILECASCADE:
		flags.is_smartly_placed = SmartPlacement(
			fw, &screen_g, fw->frame_g.width, fw->frame_g.height,
			&xl, &yt, pdeltax, pdeltay);
		if (flags.is_smartly_placed)
		{
			break;
		}
		reason->pos.has_tile_failed = 1;
		/* fall through to cascade placement */
	case PLACE_CASCADE:
	case PLACE_CASCADE_B:
		/* either "smart" placement fail and the second choice is
		 * "cascade" placement (TileCascade) or we have a "cascade"
		 * placement in any case (Cascade) or we have a crazy
		 * SPLACEMENT_MODE(sflags) value set with the old Style
		 * Dumb/Smart, Random/Active, Smart/SmartOff (i.e.:
		 * Dumb+Random+Smart or Dumb+Active+Smart) */
		if (Scr.cascade_window != NULL)
		{
			Scr.cascade_x += fw->title_thickness;
			Scr.cascade_y += 2 * fw->title_thickness;
		}
		Scr.cascade_window = fw;
		if (Scr.cascade_x > screen_g.width / 2)
		{
			Scr.cascade_x = fw->title_thickness;
		}
		if (Scr.cascade_y > screen_g.height / 2)
		{
			Scr.cascade_y = 2 * fw->title_thickness;
		}
		attr_g->x = Scr.cascade_x + PageLeft;
		attr_g->y = Scr.cascade_y + PageTop;
		/* try to keep the window on the screen */
		get_window_borders(fw, &b);
		if (attr_g->x + fw->frame_g.width >= PageRight)
		{
			attr_g->x = PageRight - attr_g->width -
				b.total_size.width;
			Scr.cascade_x = fw->title_thickness;
		}
		if (attr_g->y + fw->frame_g.height >= PageBottom)
		{
			attr_g->y = PageBottom - attr_g->height -
				b.total_size.height;
			Scr.cascade_y = 2 * fw->title_thickness;
		}

		/* the left and top sides are more important in huge
		 * windows */
		if (attr_g->x < PageLeft)
		{
			attr_g->x = PageLeft;
		}
		if (attr_g->y < PageTop)
		{
			attr_g->y = PageTop;
		}
		break;
	case PLACE_MINOVERLAP:
		CleverPlacement(
			fw, sflags, &screen_g, &xl, &yt, pdeltax, pdeltay, 0);
		flags.is_smartly_placed = True;
		break;
	case PLACE_UNDERMOUSE:
	{
		int mx;
		int my;

		if (
			FQueryPointer(
				dpy, Scr.Root, &JunkRoot, &JunkChild, &mx, &my,
				&JunkX, &JunkY, &JunkMask) == False)
		{
			/* pointer is on a different screen */
			xl = 0;
			yt = 0;
		}
		else
		{
			xl = mx - (fw->frame_g.width / 2);
			yt = my - (fw->frame_g.height / 2);
			if (
				xl + fw->frame_g.width >
				screen_g.x + screen_g.width)
			{
				xl = screen_g.x + screen_g.width -
					fw->frame_g.width;
			}
			if (
				yt + fw->frame_g.height >
				screen_g.y + screen_g.height)
			{
				yt = screen_g.y + screen_g.height -
					fw->frame_g.height;
			}
			if (xl < screen_g.x)
			{
				xl = screen_g.x;
			}
			if (yt < screen_g.y)
			{
				yt = screen_g.y;
			}
		}
		attr_g->x = xl;
		attr_g->y = yt;
		break;
	}
	default:
		/* can't happen */
		break;
	}
	if (flags.is_smartly_placed)
	{
		/* "smart" placement succed, we have done ... */
		attr_g->x = xl;
		attr_g->y = yt;
	}

	return rc;
}

static Bool __place_get_nowm_pos(
	const exec_context_t *exc, style_flags *sflags, rectangle *attr_g,
	placement_flags_t flags, rectangle screen_g,
	placement_start_style_t start_style, int mode,
	initial_window_options_t *win_opts, placement_reason_t *reason,
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
	if (SUSE_START_ON_SCREEN(sflags) && flags.do_honor_starts_on_screen)
	{
		fscreen_scr_t mangle_screen;

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
		mangle_screen = FScreenFetchMangledScreenFromUSPosHints(
			&(fw->hints));
		if (mangle_screen != FSCREEN_GLOBAL)
		{
			/* whoever set this hint knew exactly what he was
			 * doing; so ignore the StartsOnScreen style */
			flags.do_honor_starts_on_screen = 0;
			reason->pos.reason = PR_POS_USPOS_OVERRIDE_SOS;
		}
		else if (attr_g->x + attr_g->width < screen_g.x ||
			 attr_g->x >= screen_g.x + screen_g.width ||
			 attr_g->y + attr_g->height < screen_g.y ||
			 attr_g->y >= screen_g.y + screen_g.height)
		{
			/* desired coordinates do not intersect the target
			 * screen.  Let's assume the application specified
			 * global coordinates and translate them to the screen.
			 */
			FScreenTranslateCoordinates(
				NULL, start_style.screen, NULL, FSCREEN_GLOBAL,
				&attr_g->x, &attr_g->y);
			reason->pos.do_adjust_off_screen = 1;
		}
	}
	/* If SkipMapping, and other legalities are observed, adjust for
	 * StartsOnPage. */
	if (DO_NOT_SHOW_ON_MAP(fw) && flags.do_honor_starts_on_page &&
	    (!IS_TRANSIENT(fw) || SUSE_START_ON_PAGE_FOR_TRANSIENT(sflags))
#if 0
	    /* dv 08-Jul-2003:  Do not use this.  Instead, force the window on
	     * the requested page even if the application requested a different
	     * position. */
	    && (SUSE_NO_PPOSITION(sflags) || !(fw->hints.flags & PPosition))
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

	return False;
}

/* Handles initial placement and sizing of a new window
 *
 * Return value:
 *
 *   0 = window lost
 *   1 = OK
 *   2 = OK, window must be resized too */
static Bool __place_window(
	const exec_context_t *exc, style_flags *sflags, rectangle *attr_g,
	placement_start_style_t start_style, int mode,
	initial_window_options_t *win_opts, placement_reason_t *reason)
{
	FvwmWindow *t;
	int px = 0;
	int py = 0;
	int pdeltax = 0;
	int pdeltay = 0;
	rectangle screen_g;
	Bool rc = False;
	placement_flags_t flags;
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
	if (SUSE_START_ON_DESK(sflags) || SUSE_START_ON_SCREEN(sflags))
	{
		flags.do_honor_starts_on_page = 1;
		flags.do_honor_starts_on_screen = 1;
		/*
		 * Honor the flag unless...
		 * it's a restart or recapture, and that option's disallowed...
		 */
		if (win_opts->flags.do_override_ppos &&
		    (Restarting || (Scr.flags.are_windows_captured)) &&
		    !SRECAPTURE_HONORS_STARTS_ON_PAGE(sflags))
		{
			flags.do_honor_starts_on_page = 0;
			flags.do_honor_starts_on_screen = 0;
			reason->page.reason = PR_PAGE_IGNORE_CAPTURE;
			reason->page.do_ignore_starts_on_page = 1;
			reason->screen.reason = PR_PAGE_IGNORE_CAPTURE;
		}
		/*
		 * it's a cold start window capture, and that's disallowed...
		 */
		if (win_opts->flags.do_override_ppos &&
		    (!Restarting && !(Scr.flags.are_windows_captured)) &&
		    !SCAPTURE_HONORS_STARTS_ON_PAGE(sflags))
		{
			flags.do_honor_starts_on_page = 0;
			flags.do_honor_starts_on_screen = 0;
			reason->page.reason = PR_PAGE_IGNORE_CAPTURE;
			reason->page.do_ignore_starts_on_page = 1;
			reason->screen.reason = PR_PAGE_IGNORE_CAPTURE;
		}
		/*
		 * it's ActivePlacement and SkipMapping, and that's disallowed.
		 */
		if (!win_opts->flags.do_override_ppos &&
		    (DO_NOT_SHOW_ON_MAP(fw) &&
		     (SPLACEMENT_MODE(sflags) == PLACE_MANUAL ||
		      SPLACEMENT_MODE(sflags) == PLACE_MANUAL_B ||
		      SPLACEMENT_MODE(sflags) == PLACE_TILEMANUAL) &&
		     !SMANUAL_PLACEMENT_HONORS_STARTS_ON_PAGE(sflags)))
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
	if (SUSE_START_ON_SCREEN(sflags))
	{
		if (flags.do_honor_starts_on_screen)
		{
			/* use screen from style */
			FScreenGetScrRect(
				NULL, start_style.screen, &screen_g.x,
				&screen_g.y, &screen_g.width, &screen_g.height);
			reason->screen.screen = start_style.screen;
		}
		else
		{
			/* use global screen */
			FScreenGetScrRect(
				NULL, FSCREEN_GLOBAL, &screen_g.x, &screen_g.y,
				&screen_g.width, &screen_g.height);
			reason->screen.screen = FSCREEN_GLOBAL;
		}
	}
	else
	{
		/* use current screen */
		FScreenGetScrRect(
			NULL, FSCREEN_CURRENT, &screen_g.x, &screen_g.y,
			&screen_g.width, &screen_g.height);
		reason->screen.screen = FSCREEN_CURRENT;
	}

	if (SPLACEMENT_MODE(sflags) != PLACE_MINOVERLAPPERCENT &&
	    SPLACEMENT_MODE(sflags) != PLACE_MINOVERLAP)
	{
		EWMH_GetWorkAreaIntersection(
			fw, &screen_g.x, &screen_g.y, &screen_g.width,
			&screen_g.height, SEWMH_PLACEMENT_MODE(sflags));
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
	if (S_IS_STICKY_ACROSS_DESKS(SFC(*sflags)))
	{
		fw->Desk = Scr.CurrentDesk;
		reason->desk.reason = PR_DESK_STICKY;
	}
	else if (SUSE_START_ON_DESK(sflags) && start_style.desk &&
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
					reason->desk.reason = PR_DESK_TRANSIENT;
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
	if (S_IS_STICKY_ACROSS_PAGES(SFC(*sflags)))
	{
		reason->page.reason = PR_PAGE_STICKY;
	}
	else if (SUSE_START_ON_DESK(sflags))
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
	__place_get_placement_flags(&flags, fw, sflags, win_opts, mode, reason);
	if (flags.do_not_use_wm_placement)
	{
		rc = __place_get_nowm_pos(
			exc, sflags, attr_g, flags, screen_g, start_style,
			mode, win_opts, reason, pdeltax, pdeltay);
	}
	else
	{
		rc = __place_get_wm_pos(
			exc, sflags, attr_g, flags, screen_g, start_style,
			mode, win_opts, reason, pdeltax, pdeltay);
	}
	reason->pos.x = attr_g->x;
	reason->pos.y = attr_g->y;

	return rc;
}

static void __place_handle_x_resources(
	FvwmWindow *fw, window_style *pstyle, placement_reason_t *reason)
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
		&db, table, 4, client_argv[0], &client_argc, client_argv, True);
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
		SSET_START_SCREEN(
			*pstyle, FScreenGetScreenArgument(rm_value.addr, 'c'));
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

static void __explain_placement(FvwmWindow *fw, placement_reason_t *reason)
{
	char explanation[2048];
	char *r;
	char *s;
	char t[32];
	int do_show_page;
	int is_placed_by_algo;

	*explanation = 0;
	s = explanation;
	strcat(s, "placed new window 0x%x '%s':\n");
	s += strlen(s);
	sprintf(
		s, "  initial size %dx%d\n", fw->frame_g.width,
		fw->frame_g.height);
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
	if (FScreenIsEnabled() == True || FScreenIsSLSEnabled() == True)
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
		FScreenSpecToString(t, 32, reason->screen.screen);
		sprintf(
			s, "  screen: %s: %d %d %dx%d (%s)\n",
			t, reason->screen.g.x, reason->screen.g.y,
			reason->screen.g.width, reason->screen.g.height, r);
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

		switch (reason->pos.algo)
		{
		case PLACE_CENTER:
			a = "Center";
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
		case PLACE_UNDERMOUSE:
			a = "UnderMouse";
			break;
		default:
			a = "bug";
			break;
		}
		sprintf(s, ", placed by fvwm (%s)\n", r);
		s += strlen(s);
		sprintf(s, "    placement method: %s\n", a);
		s += strlen(s);
		if (reason->pos.do_not_manual_icon_placement == 1)
		{
			sprintf(s, "    (icon not placed manually)\n");
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
	Bool rc;
	const exec_context_t *exc;
	exec_context_changes_t ecc;
	placement_reason_t reason;
	placement_start_style_t start_style;

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
		exc, &pstyle->flags, attr_g, start_style, mode, win_opts,
		&reason);
	exc_destroy_context(exc);
	if (Scr.bo.do_explain_window_placement == 1)
	{
		__explain_placement(fw, &reason);
	}

	return rc;
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
