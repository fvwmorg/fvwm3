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
#include <unistd.h>
#include <string.h>

#include "fvwm.h"
#include "style.h"
#include "misc.h"
#include "screen.h"
#include "move_resize.h"
#include "virtual.h"
#include "stack.h"
#include "borders.h"

#ifndef MIN
#define MIN(A,B) ((A)<(B)? (A):(B))
#endif
#ifndef MAX
#define MAX(A,B) ((A)>(B)? (A):(B))
#endif

/*  RBW - 11/02/1998  */
static int get_next_x(FvwmWindow *t, int x, int y, int pdeltax, int pdeltay);
static int get_next_y(FvwmWindow *t, int y, int pdeltay);
static int test_fit(
  FvwmWindow *t, int test_x, int test_y, int aoimin, int pdeltax, int pdeltay);
static void CleverPlacement(
  FvwmWindow *t, int *x, int *y, int pdeltax, int pdeltay);
static void gravity_get_offsets(int grav, int *xp, int *yp);

/**/

/* With the advent of layers, the meaning of ONTOP in the following
   explanation has changed to mean any window in a higher layer. */

/* The following factors represent the amount of area that these types of
 * windows are counted as.  For example, by default the area of ONTOP windows
 * is counted 5 times as much as normal windows.  So CleverPlacement will
 * cover 5 times as much area of another window before it will cover an ONTOP
 * window.  To treat ONTOP windows the same as other windows, set this to 1.
 * To really, really avoid putting windows under ONTOP windows, set this to a
 * high value, say 1000.  A value of 5 will try to avoid ONTOP windows if
 * practical, but if it saves a reasonable amount of area elsewhere, it will
 * place one there.  The same rules apply for the other "AVOID" factors.
 * (for CleverPlacement)
 */
#define AVOIDONTOP 5
#define AVOIDSTICKY 1
#ifdef NO_STUBBORN_PLACEMENT
#define AVOIDICON 0     /*  Ignore Icons.  Place windows over them  */
#else
#define AVOIDICON 10    /*  Try hard no to place windows over icons */
#endif

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
  int loc_ok = False, tw,tx,ty,th;
  FvwmWindow *test_window;
  int stickyx, stickyy;

  if (Scr.go.SmartPlacementIsClever) /* call clever placement instead? */
  {
/*  RBW - 11/02/1998  */
    CleverPlacement(t,x,y,pdeltax,pdeltay);
/**/
    return rc;
  }
/*  RBW - 11/02/1998  */
test_x = PageLeft;
test_y = PageTop;
/**/

  temp_h = height;
  temp_w = width;

/*  RBW - 11/02/1998  */
  while(((test_y + temp_h) < (PageBottom))&&(!loc_ok))
  {
    test_x = PageLeft;
    while(((test_x + temp_w) < (PageRight))&&(!loc_ok))
    {
      loc_ok = True;
      test_window = Scr.FvwmRoot.next;
      while((test_window != (FvwmWindow *)0)&&(loc_ok == True))
      {
        /*  RBW - account for sticky windows...  */
        if(test_window->Desk == t->Desk || IS_STICKY(test_window))
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
#ifndef NO_STUBBORN_PLACEMENT
          if((IS_ICONIFIED(test_window))&&!(IS_ICON_UNMAPPED(test_window))&&
             (test_window->icon_w)&&(test_window != t))
          {
            tw=test_window->icon_p_width;
            th=test_window->icon_p_height+
              test_window->icon_w_height;
            tx = test_window->icon_x_loc - stickyx;
            ty = test_window->icon_y_loc - stickyy;

            if((tx<(test_x+width))&&((tx + tw) > test_x)&&
               (ty<(test_y+height))&&((ty + th)>test_y))
            {
              loc_ok = False;
              test_x = tx + tw;
            }
          }
#endif /* !NO_STUBBORN_PLACEMENT */
          if(!(IS_ICONIFIED(test_window))&&(test_window != t))
          {
            tw=test_window->frame_g.width;
            th=test_window->frame_g.height;
            tx = test_window->frame_g.x - stickyx;
            ty = test_window->frame_g.y - stickyy;
            if((tx <= (test_x+width))&&((tx + tw) >= test_x)&&
               (ty <= (test_y+height))&&((ty + th)>= test_y))
            {
              loc_ok = False;
              test_x = tx + tw;
            }
          }
        }
        test_window = test_window->next;
      }
      test_x +=1;
    }
    test_y +=1;
  }
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
  FvwmWindow *t, int *x, int *y, int pdeltax, int pdeltay)
{
/**/
  int test_x = 0,test_y = 0;
  int xbest, ybest;
  int aoi, aoimin;       /* area of interference */

/*  RBW - 11/02/1998  */
  int PageTop       =  0 - pdeltay;
  int PageLeft      =  0 - pdeltax;

  test_x = PageLeft;
  test_y = PageTop;
  aoi = aoimin = test_fit(t, test_x, test_y, -1, pdeltax, pdeltay);
/**/
  xbest = test_x;
  ybest = test_y;

  while((aoi != 0) && (aoi != -1))
  {
    if(aoi > 0) /* Windows interfere.  Try next x. */
    {
      test_x = get_next_x(t, test_x, test_y, pdeltax, pdeltay);
    }
    else /* Out of room in x direction. Try next y. Reset x.*/
    {
      test_x = PageLeft;
      test_y = get_next_y(t, test_y, pdeltay);
    }
    aoi = test_fit(t, test_x, test_y, aoimin, pdeltax, pdeltay);
    if((aoi >= 0) && (aoi < aoimin))
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
static int get_next_x(FvwmWindow *t, int x, int y, int pdeltax, int pdeltay)
{
/**/
  int xnew;
  int xtest;
  FvwmWindow *testw;
  int PageRight     =  Scr.MyDisplayWidth - pdeltax;
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
      if((y < (testw->icon_p_height+testw->icon_w_height+testw->icon_y_loc
	       - stickyy))&&
         (testw->icon_y_loc - stickyy < (t->frame_g.height+y)))
      {
        xtest = testw->icon_p_width+testw->icon_x_loc - stickyx;
        if(xtest > x)
          xnew = MIN(xnew, xtest);
        xtest = testw->icon_x_loc - stickyx - t->frame_g.width;
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
  return xnew;
}
/*  RBW - 11/02/1998  */
static int get_next_y(FvwmWindow *t, int y, int pdeltay)
{
/**/
  int ynew;
  int ytest;
  FvwmWindow *testw;
  int PageBottom    =  Scr.MyDisplayHeight - pdeltay;
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
      ytest = testw->icon_p_height+testw->icon_w_height+testw->icon_y_loc
	- stickyy;
      if(ytest > y)
        ynew = MIN(ynew, ytest);
      ytest = testw->icon_y_loc - stickyy - t->frame_g.height;
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
  return ynew;
}

/*  RBW - 11/02/1998  */
static int test_fit(FvwmWindow *t, int x11, int y11, int aoimin, int pdeltax,
		    int pdeltay)
{
/**/
  FvwmWindow *testw;
  int x12, x21, x22;
  int y12, y21, y22;
  int xl, xr, yt, yb; /* xleft, xright, ytop, ybottom */
  int aoi = 0;	    /* area of interference */
  int anew;
  int avoidance_factor;
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
       x21 = testw->icon_x_loc - stickyx;
       y21 = testw->icon_y_loc - stickyy;
       x22 = x21 + testw->icon_p_width;
       y22 = y21 + testw->icon_p_height + testw->icon_w_height;
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
        avoidance_factor = AVOIDICON;
      else if(compare_window_layers(testw, t) > 0)
        avoidance_factor = AVOIDONTOP;
      else if(IS_STICKY(testw))
        avoidance_factor = AVOIDSTICKY;
      else
        avoidance_factor = 1;
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
int PlaceWindow(FvwmWindow *tmp_win, style_flags *sflags, int Desk, int PageX,
		int PageY)
{
/**/
  FvwmWindow *t;
  int xl = -1,yt,DragWidth,DragHeight;
  int gravx, gravy;			/* gravity signs for positioning */
/*  RBW - 11/02/1998  */
  int px = 0, py = 0, pdeltax = 0, pdeltay = 0;
  int PageRight = Scr.MyDisplayWidth, PageBottom = Scr.MyDisplayHeight;
  int smartlyplaced       =  False;
  int rc = 1;
  Bool HonorStartsOnPage  =  False;
  extern Bool Restarting;
/**/
  extern Boolean PPosOverride;

  yt = 0;

  gravity_get_offsets(tmp_win->hints.win_gravity, &gravx, &gravy);

  /* Select a desk to put the window on (in list of priority):
   * 1. Sticky Windows stay on the current desk.
   * 2. Windows specified with StartsOnDesk go where specified
   * 3. Put it on the desk it was on before the restart.
   * 4. Transients go on the same desk as their parents.
   * 5. Window groups stay together (completely untested)
   */

/*  RBW - 11/02/1998  */
/*
    Let's get the StartsOnDesk/Page tests out of the way first.
*/
  if (SUSE_START_ON_DESK(sflags))
    {
      HonorStartsOnPage  =  True;
      /*
          Honor the flag unless...
          it's a restart or recapture, and that option's disallowed...
      */
      if (PPosOverride && (Restarting || (Scr.flags.windows_captured)) &&
	  (!Scr.go.RecaptureHonorsStartsOnPage &&
	   !SRECAPTURE_HONORS_STARTS_ON_PAGE(sflags)))
        {
          HonorStartsOnPage  =  False;
        }
     /*
          it's a cold start window capture, and that's disallowed...
      */
      if (PPosOverride && (!Restarting && !(Scr.flags.windows_captured)) &&
	  (!Scr.go.CaptureHonorsStartsOnPage &&
	   !SCAPTURE_HONORS_STARTS_ON_PAGE(sflags)))
        {
          HonorStartsOnPage  =  False;
        }
      /*
          we have a USPosition, and overriding it is disallowed...
      */
      if (!PPosOverride && (USPosition && !Scr.go.ModifyUSP))
        {
          HonorStartsOnPage  =  False;
        }
      /*
          it's ActivePlacement and SkipMapping, and that's disallowed.
      */
      if (!PPosOverride &&
	  (DO_NOT_SHOW_ON_MAP(tmp_win) &&
	   !SDO_PLACE_RANDOM(sflags) &&
	   (!Scr.go.ActivePlacementHonorsStartsOnPage &&
	    !SACTIVE_PLACEMENT_HONORS_STARTS_ON_PAGE(sflags))))
        {
          HonorStartsOnPage  =  False;
	  fvwm_msg(WARN, "PlaceWindow",
		   "illegal style combination used: StartsOnPage/StartsOnDesk"
		   " and SkipMapping don't work with ActivePlacement."
		   " Putting window on current page,"
		   " please use RandomPlacement or"
		   " ActivePlacementHonorsStartsOnPage.");
        }
    }
/**/

/*  Don't alter the existing desk location during Capture/Recapture.  */
  if (!PPosOverride) {
    tmp_win->Desk = Scr.CurrentDesk;
  }
  if (SIS_STICKY(*sflags))
    tmp_win->Desk = Scr.CurrentDesk;
  else if ((SUSE_START_ON_DESK(sflags)) && Desk && HonorStartsOnPage)
    tmp_win->Desk = (Desk > -1) ? Desk - 1 : Desk;    /*  RBW - 11/20/1998  */
  else
    {
      if((tmp_win->wmhints)&&(tmp_win->wmhints->flags & WindowGroupHint)&&
         (tmp_win->wmhints->window_group != None)&&
         (tmp_win->wmhints->window_group != Scr.Root))
      {
        /* Try to find the group leader or another window
         * in the group */
        for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
        {
          if((t->w == tmp_win->wmhints->window_group)||
             ((t->wmhints)&&(t->wmhints->flags & WindowGroupHint)&&
              (t->wmhints->window_group==tmp_win->wmhints->window_group)))
            tmp_win->Desk = t->Desk;
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
            &atype, &aformat, &nitems, &bytes_remain, &prop
          ) == Success
        ) {
          if (prop != NULL) {
            tmp_win->Desk = *(unsigned long *)prop;
            XFree(prop);
          }
        }
      }
    }

  /* I think it would be good to switch to the selected desk
   * whenever a new window pops up, except during initialization */
  if((!PPosOverride)&&(!DO_NOT_SHOW_ON_MAP(tmp_win)))
/*  RBW - 11/02/1998  --  I dont. */
  {
    changeDesks(tmp_win->Desk);
  }

/*
    Don't move viewport if SkipMapping, or if recapturing the window,
    adjust the coordinates later. Otherwise, just switch to the target
    page - it's ever so much simpler.
*/
    if (!SIS_STICKY(*sflags) && SUSE_START_ON_DESK(sflags))
    {
      if (PageX && PageY)
      {
	px  =  PageX - 1;
	py  =  PageY - 1;
	px  *= Scr.MyDisplayWidth;
	py  *= Scr.MyDisplayHeight;
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
  if (!(IS_TRANSIENT(tmp_win)) &&
      !(tmp_win->hints.flags & USPosition) &&
      (SUSE_NO_PPOSITION(sflags)||
       !(tmp_win->hints.flags & PPosition)) &&
      !(PPosOverride) &&
      /*  RBW - allow StartsOnPage to go through, even if iconic.  */
      ( ((!((tmp_win->wmhints)&&
	    (tmp_win->wmhints->flags & StateHint)&&
	    (tmp_win->wmhints->initial_state == IconicState)))
	 || (HonorStartsOnPage)) ) )
  {
    /* Get user's window placement, unless RandomPlacement is specified */
    if(SDO_PLACE_RANDOM(sflags))
    {
      if(SDO_PLACE_SMART(sflags))
        smartlyplaced = SmartPlacement(tmp_win,tmp_win->frame_g.width,
                                       tmp_win->frame_g.height,
                                       &xl,&yt, pdeltax, pdeltay);
      if(! smartlyplaced)
      {
        /* place window in a random location */
        if ((Scr.randomx += GetDecor(tmp_win,TitleHeight)) >
	    Scr.MyDisplayWidth / 2)
          Scr.randomx = GetDecor(tmp_win,TitleHeight);
        if ((Scr.randomy += 2*GetDecor(tmp_win,TitleHeight)) >
	    Scr.MyDisplayHeight / 2)
          Scr.randomy = 2 * GetDecor(tmp_win,TitleHeight);
        tmp_win->attr.x = (Scr.randomx - pdeltax) - tmp_win->old_bw;
        tmp_win->attr.y = (Scr.randomy - pdeltay) - tmp_win->old_bw;
      }
      else
      {
        tmp_win->attr.x = xl - tmp_win->old_bw;
        tmp_win->attr.y = yt - tmp_win->old_bw;
      }
      /* patches 11/93 to try to keep the window on the
       * screen */
      tmp_win->frame_g.x = tmp_win->attr.x + tmp_win->old_bw;
      tmp_win->frame_g.y = tmp_win->attr.y + tmp_win->old_bw;

      if(tmp_win->frame_g.x + tmp_win->frame_g.width +
         2*tmp_win->boundary_width> PageRight)
      {
        tmp_win->attr.x = PageRight -tmp_win->attr.width
          - tmp_win->old_bw - 2*tmp_win->boundary_width;
        Scr.randomx = 0;
      }
      if(tmp_win->frame_g.y + 2*tmp_win->boundary_width +
	 tmp_win->title_g.height + tmp_win->frame_g.height > PageBottom)
      {
        tmp_win->attr.y = PageBottom -tmp_win->attr.height
          - tmp_win->old_bw - tmp_win->title_g.height -
          2*tmp_win->boundary_width;
        Scr.randomy = 0;
      }
    }
    else
    {
      /*  Must be ActivePlacement  */
      xl = -1;
      yt = -1;
      if (SDO_PLACE_SMART(sflags))
        smartlyplaced = SmartPlacement(tmp_win,tmp_win->frame_g.width,
                                       tmp_win->frame_g.height,
                                       &xl,&yt, pdeltax, pdeltay);
      if(! smartlyplaced)
      {
        if(GrabEm(CRS_POSITION, GRAB_NORMAL))
        {
          /* Grabbed the pointer - continue */
          MyXGrabServer(dpy);
          if(XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
                          (unsigned int *)&DragWidth,
                          (unsigned int *)&DragHeight,
                          &JunkBW,  &JunkDepth) == 0)
          {
            free((char *)tmp_win);
            MyXUngrabServer(dpy);
            return False;
          }
          DragWidth = tmp_win->frame_g.width;
          DragHeight = tmp_win->frame_g.height;

          XMapRaised(dpy,Scr.SizeWindow);
	  if (moveLoop(tmp_win,0,0,DragWidth,DragHeight,&xl,&yt,False,True))
	    /* resize too */
	    rc = 2;
	  else
	    /* ok */
	    rc = 1;
          XUnmapWindow(dpy,Scr.SizeWindow);
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
      }
      /* RBW - 01/24/1999  */
      if (HonorStartsOnPage && ! smartlyplaced)
      {
        xl -= pdeltax;
        yt -= pdeltay;
      }
      /**/
      tmp_win->attr.y = yt - tmp_win->old_bw;
      tmp_win->attr.x = xl - tmp_win->old_bw;
    }
  }
  else
  {
    /* the USPosition was specified, or the window is a transient,
     * or it starts iconic so place it automatically */

/*  RBW - 11/02/1998  */
/*
    If SkipMapping, and other legalities are observed, adjust for StartsOnPage.
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
            We're placing a SkipMapping window - either capturing one that's
            previously been mapped, or overriding USPosition - so what we
            have here is its actual untouched coordinates. In case it was
            a StartsOnPage window, we have to 1) convert the existing x,y
            offsets relative to the requested page (i.e., as though there
            were only one page, no virtual desktop), then 2) readjust
            relative to the current page.
        */


          if (tmp_win->attr.x < 0)
            {
              tmp_win->attr.x  =  ((Scr.MyDisplayWidth + tmp_win->attr.x)
				   % Scr.MyDisplayWidth);
            }
          else
            {
            tmp_win->attr.x  =  tmp_win->attr.x % Scr.MyDisplayWidth;
            }
/*
    Noticed a quirk here. With some apps (e.g., xman), we find the
    placement has moved 1 pixel away from where we originally put it when we
    come through here. Why is this happening?
    Probably old_bw, try xclock -borderwidth 100
*/
          if (tmp_win->attr.y < 0)
            {
              tmp_win->attr.y = ((Scr.MyDisplayHeight + tmp_win->attr.y)
				 % Scr.MyDisplayHeight);
            }
          else
            {
              tmp_win->attr.y  =  tmp_win->attr.y % Scr.MyDisplayHeight;
            }
          tmp_win->attr.x  -=  pdeltax;
          tmp_win->attr.y  -=  pdeltay;
        }
/**/

    /* put it where asked, mod title bar */
    /* if the gravity is towards the top, move it by the title height */
    tmp_win->attr.y += gravy * tmp_win->old_bw;
    tmp_win->attr.x += gravx * tmp_win->old_bw;
    if(gravy > 0)
      tmp_win->attr.y -= 2 * tmp_win->boundary_width + tmp_win->title_g.height;
    if(gravx > 0)
      tmp_win->attr.x -= 2 * tmp_win->boundary_width;
  }
  return rc;
}


void PlaceAgain_func(F_CMD_ARGS)
{
  int x,y;
  char *token;

  if (DeferExecution(eventp, &w, &tmp_win, &context, CRS_SELECT,
		     ButtonRelease))
    return;

  /* Find new position for window */
  SmartPlacement(
    tmp_win, tmp_win->frame_g.width, tmp_win->frame_g.height, &x, &y, 0, 0);

  /* Possibly animate the movement */
  token = PeekToken(action, NULL);
  if(token && StrEquals("ANIM", token))
    AnimatedMoveFvwmWindow(tmp_win, tmp_win->frame, -1, -1, x, y, False, -1,
			   NULL);

  SetupFrame(
    tmp_win,x,y,tmp_win->frame_g.width, tmp_win->frame_g.height, False);

  return;
}


/************************************************************************
 *
 *  Procedure:
 *	gravity_get_offsets - map gravity to (x,y) offset signs for adding
 *		to x and y when window is mapped to get proper placement.
 *
 ************************************************************************/
struct _gravity_offset
{
  int x, y;
};

static void gravity_get_offsets(int grav, int *xp,int *yp)
{
  static struct _gravity_offset gravity_offsets[11] =
  {
    {  0,  0 },			/* ForgetGravity */
    { -1, -1 },			/* NorthWestGravity */
    {  0, -1 },			/* NorthGravity */
    {  1, -1 },			/* NorthEastGravity */
    { -1,  0 },			/* WestGravity */
    {  0,  0 },			/* CenterGravity */
    {  1,  0 },			/* EastGravity */
    { -1,  1 },			/* SouthWestGravity */
    {  0,  1 },			/* SouthGravity */
    {  1,  1 },			/* SouthEastGravity */
    {  0,  0 },			/* StaticGravity */
  };

  if (grav < ForgetGravity || grav > StaticGravity)
    *xp = *yp = 0;
  else
  {
    *xp = (int)gravity_offsets[grav].x;
    *yp = (int)gravity_offsets[grav].y;
  }
  return;
}

void gravity_move(int gravity, rectangle *rect, int xdiff, int ydiff)
{
  int xoff;
  int yoff;

  gravity_get_offsets(gravity, &xoff, &yoff);
  rect->x -= xoff * xdiff;
  rect->y -= yoff * ydiff;

  return;
}

void gravity_resize(int gravity, rectangle *rect, int wdiff, int hdiff)
{
  int xoff;
  int yoff;

  gravity_get_offsets(gravity, &xoff, &yoff);
  rect->x -= (wdiff * (xoff + 1)) / 2;
  rect->width += wdiff;
  rect->y -= (hdiff * (yoff + 1)) / 2;
  rect->height += hdiff;

  return;
}

void gravity_get_naked_geometry(
  int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g)
{
  int xoff;
  int yoff;

  gravity_get_offsets(gravity, &xoff, &yoff);
  dest_g->x = orig_g->x + ((xoff + 1) * (orig_g->width - 1)) / 2;
  dest_g->y = orig_g->y + ((yoff + 1) * (orig_g->height - 1)) / 2;
  dest_g->width = orig_g->width - 2 * t->boundary_width;
  dest_g->height = orig_g->height - 2 * t->boundary_width - t->title_g.height;

  return;
}

void gravity_add_decoration(
  int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g)
{
  *dest_g = *orig_g;
  gravity_resize(
    gravity, dest_g, 2 * t->boundary_width,
    2 * t->boundary_width + t->title_g.height);

  return;
}

void get_relative_geometry(rectangle *rel_g, rectangle *abs_g)
{
  rel_g->x = abs_g->x - Scr.Vx;
  rel_g->y = abs_g->y - Scr.Vy;
  rel_g->width = abs_g->width;
  rel_g->height = abs_g->height;
}

void gravity_translate_to_northwest_geometry(
  int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g)
{
  int xoff;
  int yoff;

  gravity_get_offsets(gravity, &xoff, &yoff);
  dest_g->x =
    orig_g->x - ((xoff + 1) * (orig_g->width - 1 + 2 * t->old_bw)) / 2;
  dest_g->y =
    orig_g->y - ((yoff + 1) * (orig_g->height - 1 + 2 * t->old_bw)) / 2;
  dest_g->width = orig_g->width;
  dest_g->height = orig_g->height;

  return;
}

void gravity_translate_to_northwest_geometry_no_bw(
  int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g)
{
  int bw = t->old_bw;

  t->old_bw = 0;
  gravity_translate_to_northwest_geometry(gravity, t, dest_g, orig_g);
  t->old_bw = bw;

  return;
}

void get_shaded_geometry(
  FvwmWindow *tmp_win, rectangle *small_g, rectangle *big_g)
{
  /* this variable is necessary so the function can be called with
   * small_g == big_g */
  int big_height = big_g->height;

  small_g->width = big_g->width;
  small_g->height = 2 * tmp_win->boundary_width + tmp_win->title_g.height;
  small_g->x = big_g->x;
  small_g->y = big_g->y + ((HAS_BOTTOM_TITLE(tmp_win)) ?
			   (big_height - small_g->height) : 0);
}
