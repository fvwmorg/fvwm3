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

/****************************************************************************
 * This module is all new
 * by Rob Nation
 *
 * This code does smart-placement initial window placement stuff
 *
 * Copyright 1994 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved . No guarantees or
 * warrantees of any sort whatsoever are given or implied or anything.
 ****************************************************************************/

#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include <libs/gravity.h>
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "placement.h"
#include "geometry.h"
#include "update.h"
#include "style.h"
#include "borders.h"
#include "move_resize.h"
#include "virtual.h"
#include "stack.h"
#include "add_window.h"
#include "ewmh.h"

#ifndef MIN
#define MIN(A,B) ((A)<(B)? (A):(B))
#endif
#ifndef MAX
#define MAX(A,B) ((A)>(B)? (A):(B))
#endif

static int get_next_x(
  FvwmWindow *t, rectangle *screen_g,
  int x, int y, int pdeltax, int pdeltay, int use_percent);
static int get_next_y(
  FvwmWindow *t, rectangle *screen_g, int y, int pdeltay, int use_percent);
static float test_fit(
  FvwmWindow *t, style_flags *sflags, rectangle *screen_g,
  int x11, int y11, float aoimin, int pdeltax, int pdeltay, int use_percent);
static void CleverPlacement(
  FvwmWindow *t, style_flags *sflags, rectangle *screen_g,
  int *x, int *y, int pdeltax, int pdeltay, int use_percent);

static int SmartPlacement(
  FvwmWindow *t, rectangle *screen_g,
  int width, int height, int *x, int *y, int pdeltax, int pdeltay)
{
  int PageLeft	 = screen_g->x - pdeltax;
  int PageTop	 = screen_g->y - pdeltay;
  int PageRight	 = PageLeft + screen_g->width;
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
      for ( ; test_fw != NULL && loc_ok; test_fw = test_fw->next)
      {
	if (t == test_fw || IS_EWMH_DESKTOP(FW_W(test_fw)))
	  continue;

	/*  RBW - account for sticky windows...	 */
	if (test_fw->Desk == t->Desk || IS_STICKY(test_fw))
	{
	  if (IS_STICKY(test_fw))
	  {
	    stickyx = pdeltax;
	    stickyy = pdeltay;
	  }
	  else
	  {
	    stickyx = 0;
	    stickyy = 0;
	  }
	  rc = get_visible_window_or_icon_geometry(test_fw, &g);
	  if (rc == True &&
	      (PLACEMENT_AVOID_ICON == 0 || !IS_ICONIFIED(test_fw)))
	  {
	    tx = g.x - stickyx;
	    ty = g.y - stickyy;
	    tw = g.width;
	    th = g.height;
	    if (tx < test_x + width  && test_x < tx + tw &&
		ty < test_y + height && test_y < ty + th)
	    {
	      /* window overlaps, look for a different place */
	      loc_ok = False;
	      test_x = tx + tw - 1;
	    }
	  }
	} /* if (test_fw->Desk == t->Desk || IS_STICKY(test_fw)) */
      } /* for */
      if (!loc_ok)
	test_x +=1;
    } /* while */
    if (!loc_ok)
      test_y +=1;
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
 * of interference with other windows.	If it can place a window without any
 * interference, fine.	Otherwise, it places it so that the area of of
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
    t, sflags, screen_g, test_x, test_y, -1, pdeltax, pdeltay, use_percent);
  aoimin = aoi;
  xbest = test_x;
  ybest = test_y;

  while (aoi != 0 && aoi != -1)
  {
    if (aoi > 0)
    {
      /* Windows interfere.  Try next x. */
      test_x = get_next_x(
	t, screen_g, test_x, test_y, pdeltax, pdeltay, use_percent);
    }
    else
    {
      /* Out of room in x direction. Try next y. Reset x.*/
      test_x = PageLeft;
      test_y = get_next_y(t, screen_g, test_y, pdeltay, use_percent);
    }
    aoi = test_fit(
      t, sflags, screen_g, test_x,test_y, aoimin, pdeltax, pdeltay, use_percent);
    /* I've added +0.0001 because whith my machine the < test fail with
     * certain *equal* float numbers! */
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
  FvwmWindow *t, rectangle *screen_g, int x, int y, int pdeltax, int pdeltay,
  int use_percent)
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
    start = 0;
  else
    start = GET_NEXT_STEP;

  /* Test window at far right of screen */
  xnew = PageRight;
  xtest = PageRight - t->frame_g.width;
  if (xtest > x)
    xnew = MIN(xnew, xtest);
  /* test the borders of the working area */
  xtest = PageLeft + Scr.Desktops->ewmh_working_area.x;
  if (xtest > x)
    xnew = MIN(xnew, xtest);
  xtest = PageLeft +
    (Scr.Desktops->ewmh_working_area.x + Scr.Desktops->ewmh_working_area.width)
    - t->frame_g.width;
  if (xtest > x)
    xnew = MIN(xnew, xtest);
  /* Test the values of the right edges of every window */
  for (testw = Scr.FvwmRoot.next ; testw != NULL ; testw = testw->next)
  {
    if (testw == t || (testw->Desk != t->Desk && !IS_STICKY(testw)) ||
	IS_EWMH_DESKTOP(FW_W(testw)))
      continue;

    if (IS_STICKY(testw))
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
      if (rc == True &&
	  y < g.y + g.height - stickyy && g.y - stickyy < t->frame_g.height + y)
      {
	win_left = PageLeft + g.x - stickyx - t->frame_g.width;
	for (i = start; i <= GET_NEXT_STEP; i++)
	{
	  xtest = win_left + g.width * (GET_NEXT_STEP - i) / GET_NEXT_STEP;
	  if (xtest > x)
	    xnew = MIN(xnew, xtest);
	}
	win_left = PageLeft + g.x - stickyx;
	for(i=start; i <= GET_NEXT_STEP; i++)
	{
	  xtest = (win_left) + g.width * i / GET_NEXT_STEP;
	  if (xtest > x)
	    xnew = MIN(xnew, xtest);
	}
      }
    }
    else if (y < testw->frame_g.height + testw->frame_g.y - stickyy &&
	     testw->frame_g.y - stickyy < t->frame_g.height + y)
    {
      win_left = PageLeft + testw->frame_g.x - stickyx - t->frame_g.width;
      for(i=start; i <= GET_NEXT_STEP; i++)
      {
	xtest = (win_left) + (testw->frame_g.width) *
	  (GET_NEXT_STEP - i)/GET_NEXT_STEP;
	if (xtest > x)
	  xnew = MIN(xnew, xtest);
      }
      win_left = PageLeft + testw->frame_g.x - stickyx;
      for(i=start; i <= GET_NEXT_STEP; i++)
      {
	xtest = (win_left) + (testw->frame_g.width) * i/GET_NEXT_STEP;
	if (xtest > x)
	  xnew = MIN(xnew, xtest);
      }
    }
  }

  return xnew;
}

static int get_next_y(
  FvwmWindow *t, rectangle *screen_g, int y, int pdeltay, int use_percent)
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
    start = 0;
  else
    start = GET_NEXT_STEP;

  /* Test window at far bottom of screen */
  ynew = PageBottom;
  ytest = PageBottom - t->frame_g.height;
  if (ytest > y)
    ynew = MIN(ynew, ytest);
  /* test the borders of the working area */
  ytest = screen_g->y + Scr.Desktops->ewmh_working_area.y - pdeltay;
  if (ytest > y)
    ynew = MIN(ynew, ytest);
  ytest = screen_g->y +
    (Scr.Desktops->ewmh_working_area.y + Scr.Desktops->ewmh_working_area.height)
    - t->frame_g.height;
  if (ytest > y)
    ynew = MIN(ynew, ytest);
  /* Test the values of the bottom edge of every window */
  for (testw = Scr.FvwmRoot.next ; testw != NULL ; testw = testw->next)
  {
    if (testw == t || (testw->Desk != t->Desk && !IS_STICKY(testw))
	|| IS_EWMH_DESKTOP(FW_W(testw)))
      continue;

    if (IS_STICKY(testw))
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
	  ynew = MIN(ynew, ytest);
      }
      win_top = g.y - stickyy - t->frame_g.height;
      for(i=start; i <= GET_NEXT_STEP; i++)
      {
	ytest = win_top + g.height * (GET_NEXT_STEP - i) / GET_NEXT_STEP;
	if (ytest > y)
	  ynew = MIN(ynew, ytest);
      }
    }
    else
    {
      win_top = testw->frame_g.y - stickyy;;
      for(i=start; i <= GET_NEXT_STEP; i++)
      {
	ytest = (win_top) + (testw->frame_g.height) * i/GET_NEXT_STEP;
	if (ytest > y)
	  ynew = MIN(ynew, ytest);
      }
      win_top = testw->frame_g.y - stickyy - t->frame_g.height;
      for(i=start; i <= GET_NEXT_STEP; i++)
      {
	ytest = win_top + (testw->frame_g.height) *
	  (GET_NEXT_STEP - i)/GET_NEXT_STEP;
	if (ytest > y)
	  ynew = MIN(ynew, ytest);
      }
    }
  }

  return ynew;
}

static float test_fit(
  FvwmWindow *t, style_flags *sflags, rectangle *screen_g,
  int x11, int y11, float aoimin, int pdeltax, int pdeltay, int use_percent)
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
  int PageRight	 = screen_g->x + screen_g->width - pdeltax;
  int PageBottom = screen_g->y + screen_g->height - pdeltay;
  int stickyx, stickyy;
  rectangle g;
  Bool rc;

  x12 = x11 + t->frame_g.width;
  y12 = y11 + t->frame_g.height;

  if (y12 > PageBottom) /* No room in y direction */
    return -1;
  if (x12 > PageRight) /* No room in x direction */
    return -2;
  for (testw = Scr.FvwmRoot.next ; testw != NULL ; testw = testw->next)
  {
    if (testw == t || (testw->Desk != t->Desk && !IS_STICKY(testw))
	|| IS_EWMH_DESKTOP(FW_W(testw)))
       continue;

    if (IS_STICKY(testw))
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
	avoidance_factor = ICON_PLACEMENT_PENALTY(testw);
      else if(compare_window_layers(testw, t) > 0)
	avoidance_factor = ONTOP_PLACEMENT_PENALTY(testw);
      else if(compare_window_layers(testw, t) < 0)
	avoidance_factor = BELOW_PLACEMENT_PENALTY(testw);
      else if(IS_STICKY(testw))
	avoidance_factor = STICKY_PLACEMENT_PENALTY(testw);
      else
	avoidance_factor = NORMAL_PLACEMENT_PENALTY(testw);

      if (use_percent)
      {
	cover_factor = 0;
	if ((x22 - x21) * (y22 - y21) != 0 && (x12 - x11) * (y12 - y11) != 0)
	{
	  anew = 100 * MAX(anew / ((x22 - x21) * (y22 - y21)),
			      anew / ((x12 - x11) * (y12 - y11)));
	  if (anew >= 99)
	    cover_factor = PERCENTAGE_99_PENALTY(testw);
	  else if (anew > 94)
	    cover_factor = PERCENTAGE_95_PENALTY(testw);
	  else if (anew > 84)
	    cover_factor = PERCENTAGE_85_PENALTY(testw);
	  else if (anew > 74)
	    cover_factor = PERCENTAGE_75_PENALTY(testw);
	}
	avoidance_factor += (avoidance_factor >= 1)? cover_factor : 0;
      }

      if (SEWMH_PLACEMENT_MODE(sflags) == EWMH_USE_DYNAMIC_WORKING_AREA &&
	  !DO_EWMH_IGNORE_STRUT_HINTS(testw) &&
	  (testw->dyn_strut.left > 0 || testw->dyn_strut.right > 0 ||
	   testw->dyn_strut.top > 0 || testw->dyn_strut.bottom > 0))
      {
	/* if we intersect a window which reserves space */
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
      EWMH_GetStrutIntersection(x11, y11, x12, y12, use_percent);
  }
  return aoi;
}


/**************************************************************************
 *
 * Handles initial placement and sizing of a new window
 *
 * Return value:
 *
 *   0 = window lost
 *   1 = OK
 *   2 = OK, window must be resized too
 *
 **************************************************************************/
Bool PlaceWindow(
	FvwmWindow *fw, style_flags *sflags, rectangle *attr_g,
	int Desk, int PageX, int PageY, int XineramaScreen, int mode,
	initial_window_options_type *win_opts)
{
  FvwmWindow *t;
  int xl = -1;
  int yt;
  int DragWidth;
  int DragHeight;
  int px = 0;
  int py = 0;
  int pdeltax = 0;
  int pdeltay = 0;
  int PageLeft;
  int PageTop;
  int PageRight;
  int PageBottom;
  rectangle screen_g;
  size_borders b;
  Bool rc = False;
  struct
  {
    unsigned do_forbid_manual_placement : 1;
    unsigned do_honor_starts_on_page : 1;
    unsigned do_honor_starts_on_screen : 1;
    unsigned is_smartly_placed : 1;
    unsigned do_not_use_wm_placement : 1;
  } flags;
  extern Bool Restarting;

  memset(&flags, 0, sizeof(flags));

  /* Select a desk to put the window on (in list of priority):
   * 1. Sticky Windows stay on the current desk.
   * 2. Windows specified with StartsOnDesk go where specified
   * 3. Put it on the desk it was on before the restart.
   * 4. Transients go on the same desk as their parents.
   * 5. Window groups stay together (if the KeepWindowGroupsOnDesk style is
   *	used).
   */

  /*
   *  Let's get the StartsOnDesk/Page tests out of the way first.
   */
  if (SUSE_START_ON_DESK(sflags) || SUSE_START_ON_SCREEN(sflags))
  {
    flags.do_honor_starts_on_page = True;
    flags.do_honor_starts_on_screen = True;
    /*
     * Honor the flag unless...
     * it's a restart or recapture, and that option's disallowed...
     */
    if (win_opts->flags.do_override_ppos &&
	(Restarting || (Scr.flags.windows_captured)) &&
	!SRECAPTURE_HONORS_STARTS_ON_PAGE(sflags))
    {
      flags.do_honor_starts_on_page = False;
      flags.do_honor_starts_on_screen = False;
    }
    /*
     * it's a cold start window capture, and that's disallowed...
     */
    if (win_opts->flags.do_override_ppos &&
	(!Restarting && !(Scr.flags.windows_captured)) &&
	!SCAPTURE_HONORS_STARTS_ON_PAGE(sflags))
    {
      flags.do_honor_starts_on_page = False;
      flags.do_honor_starts_on_screen = False;
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
      flags.do_honor_starts_on_page = False;
      fvwm_msg(WARN, "PlaceWindow",
	       "illegal style combination used: StartsOnPage/StartsOnDesk"
	       " and SkipMapping don't work with ManualPlacement and"
	       " TileManualPlacement. Putting window on current page,"
	       " please use an other placement style or"
	       " ActivePlacementHonorsStartsOnPage.");
    }
  } /* if (SUSE_START_ON_DESK(sflags) || SUSE_START_ON_SCREEN(sflags))) */
  /* get the screen coordinates to place window on */
  if (SUSE_START_ON_SCREEN(sflags))
  {
    if (flags.do_honor_starts_on_screen)
    {
      /* use screen from style */
      FScreenGetScrRect(
	NULL, XineramaScreen,
	&screen_g.x, &screen_g.y, &screen_g.width, &screen_g.height);
    }
    else
    {
      /* use global screen */
      FScreenGetScrRect(
	NULL, FSCREEN_GLOBAL,
	&screen_g.x, &screen_g.y, &screen_g.width, &screen_g.height);
    }
  }
  else
  {
    /* use current screen */
    FScreenGetScrRect(
      NULL, FSCREEN_CURRENT,
      &screen_g.x, &screen_g.y, &screen_g.width, &screen_g.height);
  }

  if (SPLACEMENT_MODE(sflags) != PLACE_MINOVERLAPPERCENT &&
      SPLACEMENT_MODE(sflags) != PLACE_MINOVERLAP)
  {
    EWMH_GetWorkAreaIntersection(fw,
      &screen_g.x, &screen_g.y, &screen_g.width, &screen_g.height,
      SEWMH_PLACEMENT_MODE(sflags));
  }

  PageLeft   = screen_g.x - pdeltax;
  PageTop    = screen_g.y - pdeltay;
  PageRight  = PageLeft + screen_g.width;
  PageBottom = PageTop + screen_g.height;
  yt = PageTop;

  /* Don't alter the existing desk location during Capture/Recapture.  */
  if (!win_opts->flags.do_override_ppos)
  {
    fw->Desk = Scr.CurrentDesk;
  }
  if (SIS_STICKY(*sflags))
    fw->Desk = Scr.CurrentDesk;
  else if (SUSE_START_ON_DESK(sflags) && Desk && flags.do_honor_starts_on_page)
    fw->Desk = (Desk > -1) ? Desk - 1 : Desk;
  else
  {
    if ((DO_USE_WINDOW_GROUP_HINT(fw)) &&
	(fw->wmhints) && (fw->wmhints->flags & WindowGroupHint)&&
	(fw->wmhints->window_group != None) &&
	(fw->wmhints->window_group != Scr.Root))
    {
      /* Try to find the group leader or another window in the group */
      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
      {
	if (FW_W(t) == fw->wmhints->window_group)
	{
	  /* found the group leader, break out */
	  fw->Desk = t->Desk;
	  break;
	}
	else if (t->wmhints && (t->wmhints->flags & WindowGroupHint) &&
		 (t->wmhints->window_group == fw->wmhints->window_group))
	{
	  /* found a window from the same group, but keep looking for the group
	   * leader */
	  fw->Desk = t->Desk;
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
	  dpy, FW_W(fw), _XA_WM_DESKTOP, 0L, 1L, True, _XA_WM_DESKTOP,
	  &atype, &aformat, &nitems, &bytes_remain, &prop) == Success)
      {
	if (prop != NULL)
	{
	  fw->Desk = *(unsigned long *)prop;
	  XFree(prop);
	}
      }
    }
  }

  /* I think it would be good to switch to the selected desk
   * whenever a new window pops up, except during initialization */
  /*  RBW - 11/02/1998	--  I dont. */
  if ((!win_opts->flags.do_override_ppos)&&(!DO_NOT_SHOW_ON_MAP(fw)))
  {
    goto_desk(fw->Desk);
  }

  /* Don't move viewport if SkipMapping, or if recapturing the window,
   * adjust the coordinates later. Otherwise, just switch to the target
   * page - it's ever so much simpler.
   */
  if (!SIS_STICKY(*sflags) && SUSE_START_ON_DESK(sflags))
  {
    if (PageX && PageY)
    {
      px = PageX - 1;
      py = PageY - 1;
      px *= Scr.MyDisplayWidth;
      py *= Scr.MyDisplayHeight;
      if ( (!win_opts->flags.do_override_ppos) && (!DO_NOT_SHOW_ON_MAP(fw)))
      {
	MoveViewport(px,py,True);
      }
      else if (flags.do_honor_starts_on_page)
      {
	/*  Save the delta from current page */
	pdeltax	    = Scr.Vx - px;
	pdeltay	    = Scr.Vy - py;
	PageLeft   -= pdeltax;
	PageRight  -= pdeltax;
	PageTop	   -= pdeltay;
	PageBottom -= pdeltay;
      }
    }
  }

  /* Desk has been selected, now pick a location for the window.
   *
   * Windows use the position hint if these conditions are met:
   *
   *  The program specified a USPosition hint and it is not overridden with
   *  the No(Transient)USPosition style.
   *
   * OR
   *
   *  The program specified a PPosition hint and it is not overridden with
   *  the No(Transient)PPosition style.
   *
   * Windows without a position hint are placed using wm placement.
   *
   * If RandomPlacement was specified, then place the window in a
   * psuedo-random location
   */
  {
    Bool override_ppos;
    Bool override_uspos;
    Bool has_ppos = False;
    Bool has_uspos = False;

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
    if ((fw->hints.flags & PPosition) && !override_ppos)
    {
      has_ppos = True;
    }
    if ((fw->hints.flags & USPosition) && !override_uspos)
    {
      has_uspos = True;
    }

    if (mode == PLACE_AGAIN)
    {
      flags.do_not_use_wm_placement = False;
    }
    else if (has_ppos || has_uspos)
    {
      flags.do_not_use_wm_placement = True;
    }
    else if (win_opts->flags.do_override_ppos)
    {
      flags.do_not_use_wm_placement = True;
    }
    else if (!flags.do_honor_starts_on_page &&
	     fw->wmhints && (fw->wmhints->flags & StateHint) &&
	     fw->wmhints->initial_state == IconicState)
    {
      flags.do_forbid_manual_placement = True;
    }
  }

  if (!flags.do_not_use_wm_placement)
  {
    unsigned int placement_mode = SPLACEMENT_MODE(sflags);

    /* override if Manual placement happen */
    SET_PLACED_BY_FVWM(fw,True);

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
    switch (placement_mode)
    {
    case PLACE_TILEMANUAL:
      flags.is_smartly_placed =
	SmartPlacement(
	  fw, &screen_g, fw->frame_g.width, fw->frame_g.height,
	  &xl, &yt, pdeltax, pdeltay);
      if (flags.is_smartly_placed)
	break;
      /* fall through to manual placement */
    case PLACE_MANUAL:
    case PLACE_MANUAL_B:
      /*  either "smart" placement fail and the second choice is a manual
	  placement (TileManual) or we have a manual placement in any case
	  (Manual)
      */
      xl = -1;
      yt = -1;

      if (GrabEm(CRS_POSITION, GRAB_NORMAL))
      {
	int mx;
	int my;

	/* Grabbed the pointer - continue */
	MyXGrabServer(dpy);
	if (XGetGeometry(dpy, FW_W(fw), &JunkRoot, &JunkX, &JunkY,
			 (unsigned int *)&DragWidth,
			 (unsigned int *)&DragHeight,
			 &JunkBW,  &JunkDepth) == 0)
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
	FScreenGetScrRect(NULL, FSCREEN_GLOBAL, &mx, &my, NULL, NULL);
	if (moveLoop(fw, mx, my, DragWidth, DragHeight,
		     &xl, &yt, False))
	{
	  /* resize too */
	  rc = True;
	}
	else
	{
	  /* ok */
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
      flags.is_smartly_placed =
	SmartPlacement(
	  fw, &screen_g, fw->frame_g.width, fw->frame_g.height,
	  &xl, &yt, pdeltax, pdeltay);
      if (flags.is_smartly_placed)
	break;
      /* fall through to cascade placement */
    case PLACE_CASCADE:
    case PLACE_CASCADE_B:
      /*  either "smart" placement fail and the second choice is "cascade"
	  placement (TileCascade) or we have a "cascade" placement in any case
	  (Cascade) or we have a crazy SPLACEMENT_MODE(sflags) value set with
	  the old Style Dumb/Smart, Random/Active, Smart/SmartOff (i.e.:
	  Dumb+Random+Smart or Dumb+Active+Smart)
      */
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
      attr_g->x = Scr.cascade_x + PageLeft - pdeltax;
      attr_g->y = Scr.cascade_y + PageTop - pdeltay;

      /* migo: what should these crazy calculations mean? */
#if 0
      fw->frame_g.x = PageLeft + attr_g->x + fw->attr_backup.border_width + 10;
      fw->frame_g.y = PageTop + attr_g->y + fw->attr_backup.border_width + 10;
#endif

      /* try to keep the window on the screen */
      get_window_borders(fw, &b);
      if (attr_g->x + fw->frame_g.width >= PageRight)
      {
	attr_g->x = PageRight - attr_g->width - b.total_size.width;
	Scr.cascade_x = fw->title_thickness;
      }
      if (attr_g->y + fw->frame_g.height >= PageBottom)
      {
	attr_g->y = PageBottom - attr_g->height - b.total_size.height;
	Scr.cascade_y = 2 * fw->title_thickness;
      }

      /* the left and top sides are more important in huge windows */
      if (attr_g->x < 0)
      {
	attr_g->x = 0;
      }
      if (attr_g->y < 0)
      {
	attr_g->y = 0;
      }
      break;
    case PLACE_MINOVERLAP:
      CleverPlacement(
	      fw, sflags, &screen_g, &xl, &yt, pdeltax, pdeltay, 0);
      flags.is_smartly_placed = True;
      break;
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
  } /* !flags.do_not_use_wm_placement */
  else
  {
    if (!win_opts->flags.do_override_ppos)
      SET_PLACED_BY_FVWM(fw,False);
    /* the USPosition was specified, or the window is a transient,
     * or it starts iconic so place it automatically */

    if (SUSE_START_ON_SCREEN(sflags) && flags.do_honor_starts_on_screen)
    {
      /*
       * If StartsOnScreen has been given for a window, translate its
       * USPosition so that it is relative to that particular screen.
       * If we don't do this, then a geometry would completely cancel
       * the effect of the StartsOnScreen style.
       *
       * So there are two ways to get a window to pop up on a particular
       * Xinerama screen.  1: The intuitive way giving a geometry hint
       * relative to the desired screen's 0,0 along with the appropriate
       * StartsOnScreen style (or *wmscreen resource), or 2: Do NOT
       * specify a Xinerama screen (or specify it to be 'g') and give
       * the geometry hint in terms of the global screen.
       */

      FScreenTranslateCoordinates(
	NULL, XineramaScreen, NULL, FSCREEN_GLOBAL, &attr_g->x, &attr_g->y);
    }

    /*
     *	If SkipMapping, and other legalities are observed, adjust for
     * StartsOnPage.
     */
    if ( ( DO_NOT_SHOW_ON_MAP(fw) && flags.do_honor_starts_on_page )  &&

	 ( (!(IS_TRANSIENT(fw)) ||
	    SUSE_START_ON_PAGE_FOR_TRANSIENT(sflags)) &&

	   ((SUSE_NO_PPOSITION(sflags)) ||
	    !(fw->hints.flags & PPosition)) &&

	   /*  RBW - allow StartsOnPage to go through, even if iconic.	*/
	   ( ((!((fw->wmhints)&&
		 (fw->wmhints->flags & StateHint)&&
		 (fw->wmhints->initial_state == IconicState)))
	      || (flags.do_honor_starts_on_page)) )

	   ) )
    {
      /*
       * We're placing a SkipMapping window - either capturing one that's
       * previously been mapped, or overriding USPosition - so what we
       * have here is its actual untouched coordinates. In case it was
       * a StartsOnPage window, we have to 1) convert the existing x,y
       * offsets relative to the requested page (i.e., as though there
       * were only one page, no virtual desktop), then 2) readjust
       * relative to the current page.
       */
      if (attr_g->x < 0)
      {
	attr_g->x = ((Scr.MyDisplayWidth + attr_g->x) % Scr.MyDisplayWidth);
      }
      else
      {
	attr_g->x = attr_g->x % Scr.MyDisplayWidth;
      }
      /*
       * Noticed a quirk here. With some apps (e.g., xman), we find the
       * placement has moved 1 pixel away from where we originally put it when
       * we come through here. Why is this happening?
       * Probably attr_backup.border_width, try xclock -borderwidth 100
       */
      if (attr_g->y < 0)
      {
	attr_g->y = ((Scr.MyDisplayHeight + attr_g->y) % Scr.MyDisplayHeight);
      }
      else
      {
	attr_g->y = attr_g->y % Scr.MyDisplayHeight;
      }
      attr_g->x -= pdeltax;
      attr_g->y -= pdeltay;
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
      /* Virtually all applications seem to share a common bug: they request
       * the top left pixel of their *border* as their origin instead of the
       * top left pixel of their client window, e.g. 'xterm -g +0+0' creates an
       * xterm that tries to map at (0 0) although its border (width 1) would
       * not be visible if it ran under plain X.  It should have tried to map
       * at (1 1) instead.  This clearly violates the ICCCM, but trying to
       * change this is like tilting at windmills.  So we have to add the
       * border width here. */
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
  }

  return rc;
}


void CMD_PlaceAgain(F_CMD_ARGS)
{
	char *token;
	float noMovement[1] = {1.0};
	float *ppctMovement = noMovement;
	window_style style;
	rectangle attr_g;
	XWindowAttributes attr;
	initial_window_options_type win_opts;

	if (DeferExecution(
		    eventp, &w, &fw, &context, CRS_SELECT, ButtonRelease))
	{
		return;
	}
	if (!XGetWindowAttributes(dpy, FW_W(fw), &attr))
	{
		return;
	}
	memset(&win_opts, 0, sizeof(win_opts));
	lookup_style(fw, &style);
	attr_g.x = attr.x;
	attr_g.y = attr.y;
	attr_g.width = attr.width;
	attr_g.height = attr.height;
	PlaceWindow(
		fw, &style.flags, &attr_g, SGET_START_DESK(style),
		SGET_START_PAGE_X(style), SGET_START_PAGE_Y(style),
		SGET_START_SCREEN(style), PLACE_AGAIN, &win_opts);

	/* Possibly animate the movement */
	token = PeekToken(action, NULL);
	if (token && StrEquals("Anim", token))
	{
		ppctMovement = NULL;
	}

	AnimatedMoveFvwmWindow(
		fw, FW_W_FRAME(fw), -1, -1, attr_g.x, attr_g.y, False, -1,
		ppctMovement);

	return;
}
