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

#ifndef MIN
#define MIN(A,B) ((A)<(B)? (A):(B))
#endif
#ifndef MAX
#define MAX(A,B) ((A)>(B)? (A):(B))
#endif

/*  RBW - 11/02/1998  */
static int get_next_x(FvwmWindow *t, int x, int y, int pdeltax, int pdeltay,
		      int use_percent);
static int get_next_y(FvwmWindow *t, int y, int pdeltay, int use_percent);
static float test_fit(FvwmWindow *t, int x11, int y11, float aoimin,
		      int pdeltax, int pdeltay, int use_percent);
static void CleverPlacement(FvwmWindow *t, int *x, int *y, int pdeltax,
			    int pdeltay, int use_percent);

/*  RBW - 11/02/1998  */
static int SmartPlacement(FvwmWindow *t,
			  int width,
			  int height,
			  int *x,
			  int *y,
			  int pdeltax,
			  int pdeltay)
{
  int PageBottom    =  Scr.MyDisplayHeight - pdeltay;
  int PageRight     =  Scr.MyDisplayWidth - pdeltax;
  int PageTop       =  0 - pdeltay;
  int PageLeft      =  0 - pdeltax;
  int rc            =  True;
/**/
  int temp_h,temp_w;
  int test_x = 0,test_y = 0;
  int loc_ok = False, tw = 0, tx = 0, ty = 0, th = 0;
  FvwmWindow *test_window;
  int stickyx, stickyy;
  Bool do_test;

  /*  RBW - 11/02/1998  */
  test_x = PageLeft;
  test_y = PageTop;
  /**/

  temp_h = height;
  temp_w = width;

  /*  RBW - 11/02/1998  */
  for ( ; test_y + temp_h < PageBottom && !loc_ok; )
  {
    test_x = PageLeft;
    while (test_x + temp_w < PageRight && !loc_ok)
    {
      loc_ok = True;
      test_window = Scr.FvwmRoot.next;
      for ( ; test_window != NULL && loc_ok; test_window = test_window->next)
      {
	if (t == test_window)
	  continue;
        /*  RBW - account for sticky windows...  */
        if (test_window->Desk == t->Desk || IS_STICKY(test_window))
        {
          if (IS_STICKY(test_window))
	  {
	    stickyx = pdeltax;
	    stickyy = pdeltay;
	  }
          else
	  {
	    stickyx = 0;
	    stickyy = 0;
	  }
	  do_test = False;
          if(PLACEMENT_AVOID_ICON != 0 && IS_ICONIFIED(test_window) &&
	     !IS_ICON_UNMAPPED(test_window) && test_window->icon_w)
          {
            tw=test_window->icon_p_width;
            th=test_window->icon_p_height+
              test_window->icon_g.height;
            tx = test_window->icon_g.x - stickyx;
            ty = test_window->icon_g.y - stickyy;
	    do_test = True;
          }
          else if(!IS_ICONIFIED(test_window))
          {
            tw = test_window->frame_g.width;
            th = test_window->frame_g.height;
            tx = test_window->frame_g.x - stickyx;
            ty = test_window->frame_g.y - stickyy;
	    do_test = True;
          }
	  if (do_test)
	  {
            if (tx < test_x + width  && test_x < tx + tw &&
		ty < test_y + height && test_y < ty + th)
            {
	      /* window overlaps, look for a different place */
              loc_ok = False;
              test_x = tx + tw - 1;
            }
	  }
        } /* if (test_window->Desk == t->Desk || IS_STICKY(test_window)) */
      } /* for */
      if (!loc_ok)
	test_x +=1;
    } /* while */
    if (!loc_ok)
      test_y +=1;
  } /* while */
  /*  RBW - 11/02/1998  */
  if(loc_ok == False)
  {
    rc  =  False;
    return rc;
  }
  *x = test_x;
  *y = test_y;
  return rc;
  /**/
}


/* CleverPlacement by Anthony Martin <amartin@engr.csulb.edu>
 * This function will place a new window such that there is a minimum amount
 * of interference with other windows.  If it can place a window without any
 * interference, fine.  Otherwise, it places it so that the area of of
 * interference between the new window and the other windows is minimized */
/*  RBW - 11/02/1998  */
static void CleverPlacement(
  FvwmWindow *t, int *x, int *y, int pdeltax, int pdeltay, int use_percent)
{
/**/
  int test_x = 0,test_y = 0;
  int xbest, ybest;
  float aoi, aoimin;       /* area of interference */

/*  RBW - 11/02/1998  */
  int PageTop       =  0 - pdeltay;
  int PageLeft      =  0 - pdeltax;

  test_x = PageLeft;
  test_y = PageTop;
  aoi = aoimin = test_fit(t, test_x, test_y, -1, pdeltax, pdeltay, use_percent);
/**/
  xbest = test_x;
  ybest = test_y;

  while((aoi != 0) && (aoi != -1))
  {
    if(aoi > 0) /* Windows interfere.  Try next x. */
    {
      test_x = get_next_x(t, test_x, test_y, pdeltax, pdeltay, use_percent);
    }
    else /* Out of room in x direction. Try next y. Reset x.*/
    {
      test_x = PageLeft;
      test_y = get_next_y(t, test_y, pdeltay, use_percent);
    }
    aoi = test_fit(t, test_x, test_y, aoimin, pdeltax, pdeltay, use_percent);
    /* I've added +0.00001 because whith my machine the < test fail with
       certain *equal* float numbers! */
    if((aoi >= 0) && (aoi+0.00001 < aoimin))
    {
      xbest = test_x;
      ybest = test_y;
      aoimin = aoi;
    }
  }
  *x = xbest;
  *y = ybest;
}

/*  RBW - 11/02/1998  */
static int get_next_x(FvwmWindow *t, int x, int y, int pdeltax, int pdeltay,
		      int use_percent)
{
/**/
  int xnew;
  int xtest;
  FvwmWindow *testw;
  int PageRight     =  Scr.MyDisplayWidth - pdeltax;
  int minx_inc = (int)(Scr.MyDisplayWidth/20);
  int stickyx, stickyy;

  /* Test window at far right of screen */
/*  RBW - 11/02/1998  */
  xnew = PageRight;
  xtest = PageRight - t->frame_g.width;
/**/
  if(xtest > x)
    xnew = MIN(xnew, xtest);
  /* Test the values of the right edges of every window */
  for(testw = Scr.FvwmRoot.next ; testw != NULL ; testw = testw->next)
  {
    if((testw == t) || (testw->Desk != t->Desk && !IS_STICKY(testw)))
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

    if(IS_ICONIFIED(testw))
    {
      if((y < (testw->icon_p_height+testw->icon_g.height+testw->icon_g.y
	       - stickyy))&&
         (testw->icon_g.y - stickyy < (t->frame_g.height+y)))
      {
        xtest = testw->icon_p_width+testw->icon_g.x - stickyx;
        if(xtest > x)
          xnew = MIN(xnew, xtest);
        xtest = testw->icon_g.x - stickyx - t->frame_g.width;
        if(xtest > x)
          xnew = MIN(xnew, xtest);
      }
    }
    else if((y < (testw->frame_g.height+testw->frame_g.y - stickyy)) &&
            (testw->frame_g.y - stickyy < (t->frame_g.height+y)))
    {
      xtest = testw->frame_g.width+testw->frame_g.x - stickyx;
      if(xtest > x)
        xnew = MIN(xnew, xtest);
      xtest = testw->frame_g.x - stickyx - t->frame_g.width;
      if(xtest > x)
        xnew = MIN(xnew, xtest);
    }
  }
  if (use_percent)
    xnew = MIN(xnew,x+minx_inc);

  return xnew;
}
/*  RBW - 11/02/1998  */
static int get_next_y(FvwmWindow *t, int y, int pdeltay, int use_percent)
{
/**/
  int ynew;
  int ytest;
  FvwmWindow *testw;
  int PageBottom    =  Scr.MyDisplayHeight - pdeltay;
  int miny_inc = Scr.MyDisplayHeight/20;
  int stickyy;

  /* Test window at far bottom of screen */
/*  RBW - 11/02/1998  */
  ynew = PageBottom;
  ytest = PageBottom - t->frame_g.height;
/**/
  if(ytest > y)
    ynew = MIN(ynew, ytest);
  /* Test the values of the bottom edge of every window */
  for(testw = Scr.FvwmRoot.next ; testw != NULL ; testw = testw->next)
  {
    if(testw == t || (testw->Desk != t->Desk &&	!IS_STICKY(testw)))
      continue;

    if (IS_STICKY(testw))
      {
	stickyy = pdeltay;
      }
    else
      {
	stickyy = 0;
      }

    if(IS_ICONIFIED(testw))
    {
      ytest = testw->icon_p_height+testw->icon_g.height+testw->icon_g.y
	- stickyy;
      if(ytest > y)
        ynew = MIN(ynew, ytest);
      ytest = testw->icon_g.y - stickyy - t->frame_g.height;
      if(ytest > y)
        ynew = MIN(ynew, ytest);
    }
    else
    {
      ytest = testw->frame_g.height+testw->frame_g.y - stickyy;
      if(ytest > y)
        ynew = MIN(ynew, ytest);
      ytest = testw->frame_g.y - stickyy - t->frame_g.height;
      if(ytest > y)
        ynew = MIN(ynew, ytest);
    }
  }
  if (use_percent)
    ynew = MIN(ynew,y+miny_inc);

  return ynew;
}

/*  RBW - 11/02/1998  */
static float test_fit( FvwmWindow *t, int x11, int y11, float aoimin,
		       int pdeltax, int pdeltay, int use_percent)
{
/**/
  FvwmWindow *testw;
  int x12, x21, x22;
  int y12, y21, y22;
  int xl, xr, yt, yb; /* xleft, xright, ytop, ybottom */
  float aoi = 0;      /* area of interference */
  float anew;
  float cover_factor = 1;
  float avoidance_factor;
  int PageBottom    =  Scr.MyDisplayHeight - pdeltay;
  int PageRight     =  Scr.MyDisplayWidth - pdeltax;
  int stickyx, stickyy;

  x12 = x11 + t->frame_g.width;
  y12 = y11 + t->frame_g.height;

  if (y12 > PageBottom) /* No room in y direction */
    return -1;
  if (x12 > PageRight) /* No room in x direction */
    return -2;
  for(testw = Scr.FvwmRoot.next ; testw != NULL ; testw = testw->next)
  {
    if (testw == t || (testw->Desk != t->Desk && !IS_STICKY(testw)))
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

    if(IS_ICONIFIED(testw))
    {
       if(testw->icon_w == None || IS_ICON_UNMAPPED(testw))
	  continue;
       x21 = testw->icon_g.x - stickyx;
       y21 = testw->icon_g.y - stickyy;
       x22 = x21 + testw->icon_p_width;
       y22 = y21 + testw->icon_p_height + testw->icon_g.height;
    }
    else
    {
       x21 = testw->frame_g.x - stickyx;
       y21 = testw->frame_g.y - stickyy;
       x22 = x21 + testw->frame_g.width;
       y22 = y21 + testw->frame_g.height;
    }
    if((x11 < x22) && (x12 > x21) &&
       (y11 < y22) && (y12 > y21))
    {
      /* Windows interfere */
      xl = MAX(x11, x21);
      xr = MIN(x12, x22);
      yt = MAX(y11, y21);
      yb = MIN(y12, y22);
      anew = (xr - xl) * (yb - yt);
      if(IS_ICONIFIED(testw))
        avoidance_factor = PLACEMENT_AVOID_ICON;
      else if(compare_window_layers(testw, t) > 0)
        avoidance_factor = PLACEMENT_AVOID_ONTOP;
      else if(compare_window_layers(testw, t) < 0)
        avoidance_factor = PLACEMENT_AVOID_BELOW;
      else if(IS_STICKY(testw))
        avoidance_factor = PLACEMENT_AVOID_STICKY;
      else
        avoidance_factor = 1.0;

      if (use_percent)
      {
	/* normalisation */
	if ((x22-x21)*(y22-y21) != 0 && (x12-x11)*(y12-y11) != 0)
	{
	  anew =
	    MAX(anew/((x22-x21)*(y22-y21)),anew/((x12-x11)*(y12-y11)))*100;
	  if (anew >= 99)
	    cover_factor = PLACEMENT_AVOID_COVER_99;
	  else if (anew >= 95)
	    cover_factor = PLACEMENT_AVOID_COVER_95;
	  else if (anew >= 85)
	    cover_factor = PLACEMENT_AVOID_COVER_85;
	  else if (anew >= 75)
	    cover_factor = PLACEMENT_AVOID_COVER_75;
	}
	if (avoidance_factor>1)
	  avoidance_factor =
	    avoidance_factor + (cover_factor>1) ? cover_factor : 0;
	else
	  avoidance_factor = cover_factor;
      }
      anew *= avoidance_factor;
      aoi += anew;
      if((aoi > aoimin)&&(aoimin != -1))
        return aoi;
    }
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
/*  RBW - 11/02/1998  */
Bool PlaceWindow(
  FvwmWindow *tmp_win, style_flags *sflags, int Desk, int PageX, int PageY,
  int mode)
{
/**/
  FvwmWindow *t;
  int xl = -1,yt,DragWidth,DragHeight;
  /*  RBW - 11/02/1998  */
  int px = 0, py = 0, pdeltax = 0, pdeltay = 0;
  int PageRight = Scr.MyDisplayWidth, PageBottom = Scr.MyDisplayHeight;
  int is_smartly_placed       =  False;
  Bool rc = False;
  Bool HonorStartsOnPage  =  False;
  Bool use_wm_placement = True;
  Bool do_allow_manual_placement = True;
  extern Bool Restarting;
/**/
  extern Bool PPosOverride;

  yt = 0;

  /* Select a desk to put the window on (in list of priority):
   * 1. Sticky Windows stay on the current desk.
   * 2. Windows specified with StartsOnDesk go where specified
   * 3. Put it on the desk it was on before the restart.
   * 4. Transients go on the same desk as their parents.
   * 5. Window groups stay together (if the KeepWindowGroupsOnDesk style is
   *    used).
   */

  /*  RBW - 11/02/1998  */
  /*
   *  Let's get the StartsOnDesk/Page tests out of the way first.
   */
  if (SUSE_START_ON_DESK(sflags))
  {
    HonorStartsOnPage  =  True;
    /*
     * Honor the flag unless...
     * it's a restart or recapture, and that option's disallowed...
     */
    if (PPosOverride && (Restarting || (Scr.flags.windows_captured)) &&
	!SRECAPTURE_HONORS_STARTS_ON_PAGE(sflags))
    {
      HonorStartsOnPage  =  False;
    }
    /*
     * it's a cold start window capture, and that's disallowed...
     */
    if (PPosOverride && (!Restarting && !(Scr.flags.windows_captured)) &&
	!SCAPTURE_HONORS_STARTS_ON_PAGE(sflags))
    {
      HonorStartsOnPage  =  False;
    }
    /*
     * we have a USPosition, and overriding it is disallowed...
     */
#if 0
    /* Scr.go.ModifyUSP was always 1.  How is it supposed to be set? */
    if (!PPosOverride && (USPosition && !Scr.go.ModifyUSP))
    {
      HonorStartsOnPage  =  False;
    }
#endif
    /*
     * it's ActivePlacement and SkipMapping, and that's disallowed.
     */
    if (!PPosOverride &&
	(DO_NOT_SHOW_ON_MAP(tmp_win) &&
	 (SPLACEMENT_MODE(sflags) == PLACE_MANUAL ||
	  SPLACEMENT_MODE(sflags) == PLACE_MANUAL_B ||
	  SPLACEMENT_MODE(sflags) == PLACE_TILEMANUAL) &&
         !SMANUAL_PLACEMENT_HONORS_STARTS_ON_PAGE(sflags)))
    {
      HonorStartsOnPage  =  False;
      fvwm_msg(WARN, "PlaceWindow",
	       "illegal style combination used: StartsOnPage/StartsOnDesk"
	       " and SkipMapping don't work with ManualPlacement and"
	       " TileManualPlacement. Putting window on current page,"
	       " please use an other placement style or"
	       " ActivePlacementHonorsStartsOnPage.");
    }
  } /* if (SUSE_START_ON_DESK(sflags)) */
  /**/

  /* Don't alter the existing desk location during Capture/Recapture.  */
  if (!PPosOverride)
  {
    tmp_win->Desk = Scr.CurrentDesk;
  }
  if (SIS_STICKY(*sflags))
    tmp_win->Desk = Scr.CurrentDesk;
  else if ((SUSE_START_ON_DESK(sflags)) && Desk && HonorStartsOnPage)
    tmp_win->Desk = (Desk > -1) ? Desk - 1 : Desk;    /*  RBW - 11/20/1998  */
  else
  {
    if((DO_USE_WINDOW_GROUP_HINT(tmp_win)) &&
       (tmp_win->wmhints) && (tmp_win->wmhints->flags & WindowGroupHint)&&
       (tmp_win->wmhints->window_group != None) &&
       (tmp_win->wmhints->window_group != Scr.Root))
    {
      /* Try to find the group leader or another window in the group */
      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
      {
	if (t->w == tmp_win->wmhints->window_group)
	{
	  /* found the group leader, break out */
	  tmp_win->Desk = t->Desk;
	  break;
	}
	else if (t->wmhints && (t->wmhints->flags & WindowGroupHint) &&
		 (t->wmhints->window_group == tmp_win->wmhints->window_group))
	{
	  /* found a window from the same group, but keep looking for the group
	   * leader */
	  tmp_win->Desk = t->Desk;
	}
      }
    }
    if((IS_TRANSIENT(tmp_win))&&(tmp_win->transientfor!=None)&&
       (tmp_win->transientfor != Scr.Root))
    {
      /* Try to find the parent's desktop */
      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
      {
	if(t->w == tmp_win->transientfor)
	  tmp_win->Desk = t->Desk;
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
	  dpy, tmp_win->w, _XA_WM_DESKTOP, 0L, 1L, True, _XA_WM_DESKTOP,
	  &atype, &aformat, &nitems, &bytes_remain, &prop) == Success)
      {
	if (prop != NULL)
	{
	  tmp_win->Desk = *(unsigned long *)prop;
	  XFree(prop);
	}
      }
    }
  }

  /* I think it would be good to switch to the selected desk
   * whenever a new window pops up, except during initialization */
  /*  RBW - 11/02/1998  --  I dont. */
  if ((!PPosOverride)&&(!DO_NOT_SHOW_ON_MAP(tmp_win)))
  {
    goto_desk(tmp_win->Desk);
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
      if ( (!PPosOverride) && (!DO_NOT_SHOW_ON_MAP(tmp_win)))
      {
	MoveViewport(px,py,True);
      }
      else if (HonorStartsOnPage)
      {
	/*  Save the delta from current page */
	pdeltax       =  Scr.Vx - px;
	pdeltay       =  Scr.Vy - py;
	PageRight    -=  pdeltax;
	PageBottom   -=  pdeltay;
      }
    }
  }
  /**/

  /* Desk has been selected, now pick a location for the window */
  /*
   *  If
   *     o  the window is a transient, or
   *
   *     o  a USPosition was requested
   *
   *   then put the window where requested.
   *
   *   If RandomPlacement was specified,
   *       then place the window in a psuedo-random location
   */
  if (mode == PLACE_AGAIN)
  {
    use_wm_placement = True;
  }
  else if (IS_TRANSIENT(tmp_win))
  {
    use_wm_placement = False;
  }
  else if (tmp_win->hints.flags & USPosition)
  {
    use_wm_placement = False;
  }
  else if (!SUSE_NO_PPOSITION(sflags) && (tmp_win->hints.flags & PPosition))
  {
    use_wm_placement = False;
  }
  else if (PPosOverride)
  {
    use_wm_placement = False;
  }
  else if (!((!(tmp_win->wmhints && (tmp_win->wmhints->flags & StateHint) &&
                tmp_win->wmhints->initial_state == IconicState))
             || HonorStartsOnPage))
  {
    do_allow_manual_placement = False;
#if 0
    /* DV (30-Dec-2000): Why? With this code, new windows that are started
     * iconic  will always be created where the application wishes.  Instead we
     * should use the normal placement algorithm. Right? */
    use_wm_placement = False;
#endif
  }


  if (use_wm_placement)
  {
    unsigned int placement_mode = SPLACEMENT_MODE(sflags);

    if (!do_allow_manual_placement)
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
      is_smartly_placed =
	SmartPlacement(tmp_win, tmp_win->frame_g.width, tmp_win->frame_g.height,
		       &xl, &yt, pdeltax, pdeltay);
      if (is_smartly_placed)
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
	/* Grabbed the pointer - continue */
	MyXGrabServer(dpy);
	if (XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
			 (unsigned int *)&DragWidth,
			 (unsigned int *)&DragHeight,
			 &JunkBW,  &JunkDepth) == 0)
        {
	  MyXUngrabServer(dpy);
          UngrabEm(GRAB_NORMAL);
	  return False;
	}
	MyXGrabKeyboard(dpy);
	DragWidth = tmp_win->frame_g.width;
	DragHeight = tmp_win->frame_g.height;

	XMapRaised(dpy,Scr.SizeWindow);
	if (moveLoop(tmp_win, 0, 0, DragWidth, DragHeight, &xl, &yt, False))
	{
	  /* resize too */
	  rc = True;
	}
	else
	{
	  /* ok */
	  rc = False;
	}
	XUnmapWindow(dpy, Scr.SizeWindow);
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
      /* RBW - 01/24/1999  */
      if (HonorStartsOnPage)
      {
        xl -= pdeltax;
        yt -= pdeltay;
      }
      /**/
      tmp_win->attr.y = yt;
      tmp_win->attr.x = xl;
      break;
    case PLACE_MINOVERLAPPERCENT:
      CleverPlacement(tmp_win, &xl, &yt, pdeltax, pdeltay, 1);
      is_smartly_placed = True;
      break;
    case PLACE_TILECASCADE:
      is_smartly_placed =
	SmartPlacement(tmp_win, tmp_win->frame_g.width, tmp_win->frame_g.height,
		       &xl, &yt, pdeltax, pdeltay);
      if (is_smartly_placed)
	break;
      /* fall through to cascade placement */
    case PLACE_CASCADE:
    case PLACE_CASCADE_B:
      /*  either "smart" placement fail and the second choice is "random"
	  placement (TileCascade) or we have a "random" placement in any case
	  (Cascade) or we have a crazy SPLACEMENT_MODE(sflags) value set with
	  the old Style Dumb/Smart, Random/Active, Smart/SmartOff (i.e.:
	  Dumb+Random+Smart or Dumb+Active+Smart)
      */
      if ((Scr.randomx += tmp_win->title_g.height) >
	  Scr.MyDisplayWidth / 2)
      {
	Scr.randomx = tmp_win->title_g.height;
      }
      if ((Scr.randomy += 2 * tmp_win->title_g.height) >
	  Scr.MyDisplayHeight / 2)
      {
	Scr.randomy = 2 * tmp_win->title_g.height;
      }
      tmp_win->attr.x = Scr.randomx - pdeltax;
      tmp_win->attr.y = Scr.randomy - pdeltay;
      /* patches 11/93 to try to keep the window on the
       * screen */
      tmp_win->frame_g.x = tmp_win->attr.x + tmp_win->old_bw + 10;
      tmp_win->frame_g.y = tmp_win->attr.y + tmp_win->old_bw + 10;

      if(tmp_win->attr.x + tmp_win->frame_g.width >= PageRight)
      {
	tmp_win->attr.x = PageRight -tmp_win->attr.width
	  - tmp_win->old_bw - 2*tmp_win->boundary_width;
	Scr.randomx = 0;
      }
      if(tmp_win->attr.y + tmp_win->frame_g.height >= PageBottom)
      {
	tmp_win->attr.y = PageBottom -tmp_win->attr.height
	  - tmp_win->old_bw - tmp_win->title_g.height -
	  2*tmp_win->boundary_width;
	Scr.randomy = 0;
      }
      break;
    case PLACE_MINOVERLAP:
      CleverPlacement(tmp_win, &xl, &yt, pdeltax, pdeltay, 0);
      is_smartly_placed = True;
      break;
    default:
      /* can't happen */
      break;
    }

    if (is_smartly_placed)
    {
      /* "smart" placement succed, we have done ... */
      tmp_win->attr.x = xl;
      tmp_win->attr.y = yt;
    }
  } /* use_wm_placement */
  else
  {
    /* the USPosition was specified, or the window is a transient,
     * or it starts iconic so place it automatically */

    /*  RBW - 11/02/1998  */
    /*
     *  If SkipMapping, and other legalities are observed, adjust for
     * StartsOnPage.
     */
    if ( ( DO_NOT_SHOW_ON_MAP(tmp_win) && HonorStartsOnPage )  &&

	 ( (!(IS_TRANSIENT(tmp_win)) ||
	    SUSE_START_ON_PAGE_FOR_TRANSIENT(sflags)) &&

	   ((SUSE_NO_PPOSITION(sflags)) ||
	    !(tmp_win->hints.flags & PPosition)) &&

           /*  RBW - allow StartsOnPage to go through, even if iconic.  */
           ( ((!((tmp_win->wmhints)&&
		 (tmp_win->wmhints->flags & StateHint)&&
		 (tmp_win->wmhints->initial_state == IconicState)))
              || (HonorStartsOnPage)) )

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
      if (tmp_win->attr.x < 0)
      {
	tmp_win->attr.x =
	  ((Scr.MyDisplayWidth + tmp_win->attr.x) % Scr.MyDisplayWidth);
      }
      else
      {
	tmp_win->attr.x = tmp_win->attr.x % Scr.MyDisplayWidth;
      }
      /*
       * Noticed a quirk here. With some apps (e.g., xman), we find the
       * placement has moved 1 pixel away from where we originally put it when
       * we come through here. Why is this happening?
       * Probably old_bw, try xclock -borderwidth 100
       */
      if (tmp_win->attr.y < 0)
      {
	tmp_win->attr.y =
	  ((Scr.MyDisplayHeight + tmp_win->attr.y) % Scr.MyDisplayHeight);
      }
      else
      {
	tmp_win->attr.y = tmp_win->attr.y % Scr.MyDisplayHeight;
      }
      tmp_win->attr.x -= pdeltax;
      tmp_win->attr.y -= pdeltay;
    }
/**/

    /* put it where asked, mod title bar */
    /* if the gravity is towards the top, move it by the title height */
    {
      rectangle final_g;
      int gravx;
      int gravy;

      gravity_get_offsets(tmp_win->hints.win_gravity, &gravx, &gravy);
      final_g.x = tmp_win->attr.x + gravx * tmp_win->old_bw;
      final_g.y = tmp_win->attr.y + gravy * tmp_win->old_bw;
      /* Virtually all applications seem to share a common bug: they request
       * the top left pixel of their *border* as their origin instead of the
       * top left pixel of their client window, e.g. 'xterm -g +0+0' creates an
       * xterm that tries to map at (0 0) although its border (width 1) would
       * not be visible if it ran under plain X.  It should have tried to map
       * at (1 1) instead.  This clearly violates the ICCCM, but trying to
       * change this is like tilting at windmills.  So we have to add the
       * border width here. */
      final_g.x += tmp_win->old_bw;
      final_g.y += tmp_win->old_bw;
      final_g.width = 0;
      final_g.height = 0;
      if (mode == PLACE_INITIAL)
      {
        gravity_resize(
          tmp_win->hints.win_gravity, &final_g,
          2 * tmp_win->boundary_width,
          2 * tmp_win->boundary_width + tmp_win->title_g.height);
      }
      tmp_win->attr.x = final_g.x;
      tmp_win->attr.y = final_g.y;
    }
  }

  return rc;
}


void CMD_PlaceAgain(F_CMD_ARGS)
{
  int x;
  int y;
  char *token;
  float noMovement[1] = {1.0};
  float *ppctMovement = noMovement;
  window_style style;

  if (DeferExecution(eventp, &w, &tmp_win, &context, CRS_SELECT,
		     ButtonRelease))
    return;

  lookup_style(tmp_win, &style);
  PlaceWindow(tmp_win, &style.flags, SGET_START_DESK(style),
	      SGET_START_PAGE_X(style), SGET_START_PAGE_Y(style), PLACE_AGAIN);
  x = tmp_win->attr.x;
  y = tmp_win->attr.y;

  /* Possibly animate the movement */
  token = PeekToken(action, NULL);
  if(token && StrEquals("ANIM", token))
    ppctMovement = NULL;

  AnimatedMoveFvwmWindow(tmp_win, tmp_win->frame, -1, -1, x, y, False, -1,
			 ppctMovement);

  return;
}
