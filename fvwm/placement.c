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
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"

#ifndef MIN
#define MIN(A,B) ((A)<(B)? (A):(B))
#endif
#ifndef MAX
#define MAX(A,B) ((A)>(B)? (A):(B))
#endif

/*  RBW - 11/02/1998  */
int get_next_x(FvwmWindow *t, int x, int y, int pdeltax, int pdeltay);
int get_next_y(FvwmWindow *t, int y, int pdeltay);
int test_fit(FvwmWindow *t, int test_x, int test_y, int aoimin, int pdeltax, int pdeltay);
void CleverPlacement(FvwmWindow *t, int *x, int *y, int pdeltax, int pdeltay);
/**/

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
int SmartPlacement(FvwmWindow *t,
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

  if (Scr.SmartPlacementIsClever) /* call clever placement instead? */
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
        if(test_window->Desk == t->Desk || (test_window->flags & STICKY))
        {
          if (test_window->flags & STICKY)
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
          if((test_window->flags & ICONIFIED)&&
             (!(test_window->flags & ICON_UNMAPPED))&&
             (test_window->icon_w)&&
             (test_window != t))
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
          if(!(test_window->flags & ICONIFIED)&&(test_window != t))
          {
            tw=test_window->frame_width+2*test_window->bw;
            th=test_window->frame_height+2*test_window->bw;
            tx = test_window->frame_x - stickyx;
            ty = test_window->frame_y - stickyy;
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
void CleverPlacement(FvwmWindow *t, int *x, int *y, int pdeltax, int pdeltay)
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
int get_next_x(FvwmWindow *t, int x, int y, int pdeltax, int pdeltay)
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
  xtest = PageRight - (t->frame_width  + 2 * t->bw);
/**/
  if(xtest > x)
    xnew = MIN(xnew, xtest);
  /* Test the values of the right edges of every window */
  for(testw = Scr.FvwmRoot.next ; testw != NULL ; testw = testw->next)
  {
    if((testw == t) || ((testw->Desk != t->Desk) && (! (testw->flags & STICKY))))
      continue;

    if (testw->flags & STICKY)
      {
        stickyx = pdeltax;
	stickyy = pdeltay;
      }
    else
      {
        stickyx = 0;
	stickyy = 0;
      }

    if(testw->flags & ICONIFIED)
    {
      if((y < (testw->icon_p_height+testw->icon_w_height+testw->icon_y_loc - stickyy))&&
         (testw->icon_y_loc - stickyy < (t->frame_height+2*t->bw+y)))
      {
        xtest = testw->icon_p_width+testw->icon_x_loc - stickyx;
        if(xtest > x)
          xnew = MIN(xnew, xtest);
        xtest = testw->icon_x_loc - stickyx - (t->frame_width + 2 * t->bw);
        if(xtest > x)
          xnew = MIN(xnew, xtest);
      }
    }
    else if((y < (testw->frame_height+2*testw->bw+testw->frame_y - stickyy)) &&
            (testw->frame_y - stickyy < (t->frame_height+2*t->bw+y)))
    {
      xtest = testw->frame_width+2*testw->bw+testw->frame_x - stickyx;
      if(xtest > x)
        xnew = MIN(xnew, xtest);
      xtest = testw->frame_x - stickyx - (t->frame_width + 2 * t->bw);
      if(xtest > x)
        xnew = MIN(xnew, xtest);
    }
  }
  return xnew;
}
/*  RBW - 11/02/1998  */
int get_next_y(FvwmWindow *t, int y, int pdeltay)
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
  ytest = PageBottom - (t->frame_height + 2 * t->bw);
/**/
  if(ytest > y)
    ynew = MIN(ynew, ytest);
  /* Test the values of the bottom edge of every window */
  for(testw = Scr.FvwmRoot.next ; testw != NULL ; testw = testw->next)
  {
    if((testw == t) || ((testw->Desk != t->Desk) && (! (testw->flags & STICKY))))
      continue;

    if (testw->flags & STICKY)
      {
	stickyy = pdeltay;
      }
    else
      {
	stickyy = 0;
      }

    if(testw->flags & ICONIFIED)
    {
      ytest = testw->icon_p_height+testw->icon_w_height+testw->icon_y_loc - stickyy;
      if(ytest > y)
        ynew = MIN(ynew, ytest);
      ytest = testw->icon_y_loc - stickyy - (t->frame_height + 2 * t->bw);
      if(ytest > y)
        ynew = MIN(ynew, ytest);
    }
    else
    {
      ytest = testw->frame_height+2*testw->bw+testw->frame_y - stickyy;
      if(ytest > y)
        ynew = MIN(ynew, ytest);
      ytest = testw->frame_y - stickyy - (t->frame_height + 2 * t->bw);
      if(ytest > y)
        ynew = MIN(ynew, ytest);
    }
  }
  return ynew;
}

/*  RBW - 11/02/1998  */
int test_fit(FvwmWindow *t, int x11, int y11, int aoimin, int pdeltax,
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

  x12 = x11 + t->frame_width  + 2 * t->bw;
  y12 = y11 + t->frame_height + 2 * t->bw;

  if (y12 > PageBottom) /* No room in y direction */
    return -1;
  if (x12 > PageRight) /* No room in x direction */
    return -2;
  for(testw = Scr.FvwmRoot.next ; testw != NULL ; testw = testw->next)
  {
    if ((testw == t) || ((testw->Desk != t->Desk) && (! (testw->flags & STICKY))))
       continue;

    if (testw->flags & STICKY)
      {
        stickyx = pdeltax;
	stickyy = pdeltay;
      }
    else
      {
        stickyx = 0;
	stickyy = 0;
      }

    if(testw->flags & ICONIFIED)
    {
       if(testw->icon_w == None || testw->flags & ICON_UNMAPPED)
	  continue;
       x21 = testw->icon_x_loc - stickyx;
       y21 = testw->icon_y_loc - stickyy;
       x22 = x21 + testw->icon_p_width;
       y22 = y21 + testw->icon_p_height + testw->icon_w_height;
    }
    else
    {
       x21 = testw->frame_x - stickyx;
       y21 = testw->frame_y - stickyy;
       x22 = x21 + testw->frame_width  + 2 * testw->bw;
       y22 = y21 + testw->frame_height + 2 * testw->bw;
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
      if(testw->flags & ICONIFIED)
        avoidance_factor = AVOIDICON;
      else if(testw->flags & ONTOP)
        avoidance_factor = AVOIDONTOP;
      else if(testw->flags & STICKY)
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
 * Returns False in the event of a lost window.
 *
 **************************************************************************/
/*  RBW - 11/02/1998  */
Bool PlaceWindow(FvwmWindow *tmp_win, unsigned long tflag,int Desk, int PageX, int PageY)
{
/**/
  FvwmWindow *t;
  int xl = -1,yt,DragWidth,DragHeight;
  int gravx, gravy;			/* gravity signs for positioning */
/*  RBW - 11/02/1998  */
  int px = 0, py = 0, pdeltax = 0, pdeltay = 0;
  int PageRight = Scr.MyDisplayWidth, PageBottom = Scr.MyDisplayHeight;
  int smartlyplaced       =  False;
  Bool HonorStartsOnPage  =  False;
  extern Bool Restarting;
/**/
  extern Boolean PPosOverride;

  yt = 0;

  GetGravityOffsets (tmp_win, &gravx, &gravy);


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
  if (tflag & STARTSONDESK_FLAG)
    {
      HonorStartsOnPage  =  True;
      /*
          Honor the flag unless...
          it's a restart or recapture, and that option's disallowed...
      */
      if (PPosOverride && (Restarting || (Scr.flags & WindowsCaptured)) &&
	  !Scr.go.RecaptureHonorsStartsOnPage)
        {
          HonorStartsOnPage  =  False;
        }
     /*
          it's a cold start window capture, and that's disallowed...
      */
      if (PPosOverride && (!Restarting && !(Scr.flags & WindowsCaptured)) &&
	  !Scr.go.CaptureHonorsStartsOnPage)
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
      if (!PPosOverride && ((tmp_win->flags & SHOW_ON_MAP) &&
          (!(tflag & RANDOM_PLACE_FLAG)) &&
			    !Scr.go.ActivePlacementHonorsStartsOnPage))
        {
          HonorStartsOnPage  =  False;
        }
    }
/**/

  tmp_win->Desk = Scr.CurrentDesk;
  if (tflag & STICKY_FLAG)
    tmp_win->Desk = Scr.CurrentDesk;
  else if ((tflag & STARTSONDESK_FLAG) && Desk && HonorStartsOnPage)
    tmp_win->Desk = (Desk > -1) ? Desk - 1 : Desk;    /*  RBW - 11/20/1998  */
  else
  {
    Atom atype;
    int aformat;
    unsigned long nitems, bytes_remain;
    unsigned char *prop;

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
    if((tmp_win->flags & TRANSIENT)&&(tmp_win->transientfor!=None)&&
       (tmp_win->transientfor != Scr.Root))
    {
      /* Try to find the parent's desktop */
      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
      {
        if(t->w == tmp_win->transientfor)
          tmp_win->Desk = t->Desk;
      }
    }

    if ((XGetWindowProperty(dpy, tmp_win->w, _XA_WM_DESKTOP, 0L, 1L, True,
                            _XA_WM_DESKTOP, &atype, &aformat, &nitems,
                            &bytes_remain, &prop))==Success)
    {
      if(prop != NULL)
      {
        tmp_win->Desk = *(unsigned long *)prop;
        XFree(prop);
      }
    }
  }
  /* I think it would be good to switch to the selected desk
   * whenever a new window pops up, except during initialization */
  if((!PPosOverride)&&(!(tmp_win->flags & SHOW_ON_MAP)))
/*  RBW - 11/02/1998  --  I dont. */
    {
      changeDesks(tmp_win->Desk);
    }

/*
    Don't move viewport if SkipMapping, or if recapturing the window,
    adjust the coordinates later. Otherwise, just switch to the target
    page - it's ever so much simpler.
*/
    if (!(tflag & STICKY_FLAG) && (tflag & STARTSONDESK_FLAG))
      {
        if (PageX && PageY)
        {
          px  =  PageX - 1;
          py  =  PageY -1 ;
          px  *= Scr.MyDisplayWidth;
          py  *= Scr.MyDisplayHeight;
          if ( (!PPosOverride) && (!(tmp_win->flags & SHOW_ON_MAP)) )
            {
              MoveViewport(px,py,True);
            }
          else
            {
              if (HonorStartsOnPage)
                {
                  /*  Save the delta from current page */
                  pdeltax       =  Scr.Vx - px;
                  pdeltay       =  Scr.Vy - py;
                  PageRight    -=  pdeltax;
                  PageBottom   -=  pdeltay;
                }
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
  if (!(tmp_win->flags & TRANSIENT) &&
      !(tmp_win->hints.flags & USPosition) &&
      ((tflag & NO_PPOSITION_FLAG)||
       !(tmp_win->hints.flags & PPosition)) &&
      !(PPosOverride) &&
      /*  RBW - allow StartsOnPage to go through, even if iconic.  */
      ( ((!((tmp_win->wmhints)&&
	    (tmp_win->wmhints->flags & StateHint)&&
	    (tmp_win->wmhints->initial_state == IconicState))) 
	 || (HonorStartsOnPage)) ) )
  {
    /* Get user's window placement, unless RandomPlacement is specified */
    if(tflag & RANDOM_PLACE_FLAG)
    {
      if(tflag & SMART_PLACE_FLAG)
        smartlyplaced = SmartPlacement(tmp_win,tmp_win->frame_width+2*tmp_win->bw,
                                       tmp_win->frame_height+2*tmp_win->bw,
                                       &xl,&yt, pdeltax, pdeltay);
      if(! smartlyplaced)
      {
        /* place window in a random location */
        if ((Scr.randomx += GetDecor(tmp_win,TitleHeight)) > Scr.MyDisplayWidth / 2)
          Scr.randomx = GetDecor(tmp_win,TitleHeight);
        if ((Scr.randomy += 2*GetDecor(tmp_win,TitleHeight)) > Scr.MyDisplayHeight / 2)
          Scr.randomy = 2 * GetDecor(tmp_win,TitleHeight);
        tmp_win->attr.x = (Scr.randomx - pdeltax) - tmp_win->old_bw;
        tmp_win->attr.y = (Scr.randomy - pdeltay) - tmp_win->old_bw;
      }
      else
      {
        tmp_win->attr.x = xl - tmp_win->old_bw + tmp_win->bw;
        tmp_win->attr.y = yt - tmp_win->old_bw + tmp_win->bw;
      }
      /* patches 11/93 to try to keep the window on the
       * screen */
      tmp_win->frame_x = tmp_win->attr.x + tmp_win->old_bw - tmp_win->bw;
      tmp_win->frame_y = tmp_win->attr.y + tmp_win->old_bw - tmp_win->bw;

      if(tmp_win->frame_x + tmp_win->frame_width +
         2*tmp_win->boundary_width> PageRight)
      {
        tmp_win->attr.x = PageRight -tmp_win->attr.width
          - tmp_win->old_bw +tmp_win->bw - 2*tmp_win->boundary_width;
        Scr.randomx = 0;
      }
      if(tmp_win->frame_y + 2*tmp_win->boundary_width+tmp_win->title_height
         + tmp_win->frame_height > PageBottom)
      {
        tmp_win->attr.y = PageBottom -tmp_win->attr.height
          - tmp_win->old_bw +tmp_win->bw - tmp_win->title_height -
          2*tmp_win->boundary_width;;
        Scr.randomy = 0;
      }

      tmp_win->xdiff = tmp_win->attr.x - tmp_win->bw;
      tmp_win->ydiff = tmp_win->attr.y - tmp_win->bw;
    }
    else
    {
      /*  Must be ActivePlacement  */
      xl = -1;
      yt = -1;
      if(tflag & SMART_PLACE_FLAG)
        smartlyplaced = SmartPlacement(tmp_win,tmp_win->frame_width+2*tmp_win->bw,
                                       tmp_win->frame_height+2*tmp_win->bw,
                                       &xl,&yt, pdeltax, pdeltay);
      if(! smartlyplaced)
      {
        if(GrabEm(POSITION))
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
          DragWidth = tmp_win->frame_width;
          DragHeight = tmp_win->frame_height;

          XMapRaised(dpy,Scr.SizeWindow);
          moveLoop(tmp_win,0,0,DragWidth,DragHeight,&xl,&yt,False,True);
          XUnmapWindow(dpy,Scr.SizeWindow);
          MyXUngrabServer(dpy);
          UngrabEm();
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
      tmp_win->attr.y = yt - tmp_win->old_bw + tmp_win->bw;
      tmp_win->attr.x = xl - tmp_win->old_bw + tmp_win->bw;
      tmp_win->xdiff = xl ;
      tmp_win->ydiff = yt ;
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

      if ( ( (tmp_win->flags & SHOW_ON_MAP) && HonorStartsOnPage )  &&

           ( !(tmp_win->flags & TRANSIENT) &&

             ((tflag & NO_PPOSITION_FLAG) ||
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
              tmp_win->attr.x  =  ((Scr.MyDisplayWidth + tmp_win->attr.x) % Scr.MyDisplayWidth);
            }
          else
            {
            tmp_win->attr.x  =  tmp_win->attr.x % Scr.MyDisplayWidth;
            }
/*
    Noticed a quirk here. With some apps (e.g., xman), we find the
    placement has moved 1 pixel away from where we originally put it when we
    come through here. Why is this happening?
*/
          if (tmp_win->attr.y < 0)
            {
              tmp_win->attr.y  =  ((Scr.MyDisplayHeight + tmp_win->attr.y) % Scr.MyDisplayHeight);
            }
          else
            {
              tmp_win->attr.y  =  tmp_win->attr.y % Scr.MyDisplayHeight;
            }
          tmp_win->attr.x  -=  pdeltax;
          tmp_win->attr.y  -=  pdeltay;
        }
/**/

    tmp_win->xdiff = tmp_win->attr.x;
    tmp_win->ydiff =  tmp_win->attr.y;
    /* put it where asked, mod title bar */
    /* if the gravity is towards the top, move it by the title height */
    tmp_win->attr.y -= gravy*(tmp_win->bw-tmp_win->old_bw);
    tmp_win->attr.x -= gravx*(tmp_win->bw-tmp_win->old_bw);
    if(gravy > 0)
      tmp_win->attr.y -= 2*tmp_win->boundary_width + tmp_win->title_height;
    if(gravx > 0)
      tmp_win->attr.x -= 2*tmp_win->boundary_width;
  }
  return True;
}



/************************************************************************
 *
 *  Procedure:
 *	GetGravityOffsets - map gravity to (x,y) offset signs for adding
 *		to x and y when window is mapped to get proper placement.
 *
 ************************************************************************/
struct _gravity_offset
{
  int x, y;
};

void GetGravityOffsets (FvwmWindow *tmp,int *xp,int *yp)
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
  register int g = ((tmp->hints.flags & PWinGravity)
		    ? tmp->hints.win_gravity : NorthWestGravity);

  if (g < ForgetGravity || g > StaticGravity)
    *xp = *yp = 0;
  else
  {
    *xp = (int)gravity_offsets[g].x;
    *yp = (int)gravity_offsets[g].y;
  }
  return;
}
