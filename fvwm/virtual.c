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

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "fvwm.h"
#include "icons.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

/*
 * dje 12/19/98
 *
 * Testing with edge_thickness of 1  showed that some XServers don't
 * care for 1 pixel windows, causing EdgeScrolling  not to work with some
 * servers.  One  bug report was for SUNOS  4.1.3_U1.  We didn't find out
 * the exact  cause of the problem.  Perhaps  no enter/leave  events were
 * generated.
 *
 * Allowed: 0,1,or 2 pixel pan frames.
 *
 * 0   completely disables mouse  edge scrolling,  even  while dragging a
 * window.
 *
 * 1 gives the  smallest pan frames,  which seem to  work best  except on
 * some servers.
 *
 * 2 is the default.
 */
static int edge_thickness = 2;
static int last_edge_thickness = 2;
static unsigned int prev_page_x = 0;
static unsigned int prev_page_y = 0;

/**************************************************************************
 *
 * Parse arguments for "Desk" and "MoveToDesk" (formerly "WindowsDesk"):
 *
 * (nil)       : desk number = current desk
 * n           : desk number = current desk + n
 * 0 n         : desk number = n
 * n x         : desk number = current desk + n
 * 0 n min max : desk number = n, but limit to min/max
 * n min max   : desk number = current desk + n, but wrap around at desk #min
 *               or desk #max
 * n x min max : desk number = current desk + n, but wrap around at desk #min
 *               or desk #max
 *
 * The current desk number is returned if not enough parameters could be
 * read (or if action is empty).
 *
 **************************************************************************/
static int GetDeskNumber(char *action)
{
  int n;
  int m;
  int desk;
  int val[4];
  int min, max;

  n = GetIntegerArguments(action, NULL, &(val[0]), 4);
  if (n <= 0)
    return Scr.CurrentDesk;
  if (n == 1)
    return Scr.CurrentDesk + val[0];

  desk = Scr.CurrentDesk;
  m = 0;

  if (val[0] == 0)
    {
      /* absolute desk number */
      desk = val[1];
    }
  else
    {
      /* relative desk number */
      desk += val[0];
    }

  if (n == 3)
    {
      m = 1;
    }
  if (n == 4)
    {
      m = 2;
    }


  if (n > 2)
    {
      /* handle limits */
      if (val[m] <= val[m+1])
	{
	  min = val[m];
	  max = val[m+1];
	}
      else
	{
          /*  min > max is nonsense, so swap 'em.  */
	  min = val[m+1];
	  max = val[m];
	}
      if (desk < min)
	{
          /*  Relative move outside of range, wrap around.  */
	  if (val[0] < 0)
	    desk = max;
	  else
	    desk = min;
	}
      else if (desk > max)
	{
          /*  Relative move outside of range, wrap around.  */
	  if (val[0] > 0)
	    desk = min;
	  else
	    desk = max;
	}
    }

  return desk;
}



void setEdgeThickness(F_CMD_ARGS)
{
  int val, n;

  n = GetIntegerArguments(action, NULL, &val, 1);
  if(n != 1) {
    fvwm_msg(ERR,"setEdgeThickness",
             "EdgeThickness requires 1 numeric argument, found %d args",n);
    return;
  }
  if (val < 0 || val > 2) {             /* check range */
    fvwm_msg(ERR,"setEdgeThickness",
             "EdgeThickness arg must be between 0 and 2, found %d",val);
    return;
  }
  edge_thickness = val;
  checkPanFrames();
}

/***************************************************************************
 *
 * Check to see if the pointer is on the edge of the screen, and scroll/page
 * if needed
 ***************************************************************************/
Bool HandlePaging(int HorWarpSize, int VertWarpSize, int *xl, int *yt,
		  int *delta_x, int *delta_y,Bool Grab, Bool fLoop)
{
  static unsigned int add_time = 0;
  int x,y;
  static Time my_timestamp = 0;

  *delta_x = 0;
  *delta_y = 0;

  if((Scr.ScrollResistance >= 10000)||
     ((HorWarpSize ==0)&&(VertWarpSize==0)))
    {
      my_timestamp = 0;
      add_time = 0;
      return False;
    }

  /* need to move the viewport */
  if(Scr.VxMax == 0 && Scr.VyMax == 0)
    {
      my_timestamp = 0;
      add_time = 0;
      return False;
    }

  if (my_timestamp == 0)
    {
      my_timestamp = lastTimestamp;
      add_time = 0;
    }

  do
    {
      if(XCheckWindowEvent(dpy,Scr.PanFrameTop.win, LeaveWindowMask,&Event)  ||
	 XCheckWindowEvent(dpy,Scr.PanFrameBottom.win,LeaveWindowMask,&Event)||
	 XCheckWindowEvent(dpy,Scr.PanFrameLeft.win, LeaveWindowMask,&Event) ||
	 XCheckWindowEvent(dpy,Scr.PanFrameRight.win, LeaveWindowMask,&Event))
	{
	  StashEventTime(&Event);
	  my_timestamp = 0;
	  add_time = 0;
	  return False;
	}
      XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &x, &y, &JunkX, &JunkY, &JunkMask);
      /* check actual pointer location since PanFrames can get buried under
	 a window being moved or resized - mab */
      if(( x >= edge_thickness )&&( x < Scr.MyDisplayWidth-edge_thickness )&&
	 ( y >= edge_thickness )&&( y < Scr.MyDisplayHeight-edge_thickness ))
	{
	  my_timestamp = 0;
	  add_time = 0;
	  return False;
	}
      usleep(10000);
      add_time += 10;
    } while (fLoop &&
	     lastTimestamp - my_timestamp + add_time < Scr.ScrollResistance);

  if (lastTimestamp - my_timestamp + add_time < Scr.ScrollResistance)
{
    return False;
}
  /* Move the viewport */
  /* and/or move the cursor back to the approximate correct location */
  /* that is, the same place on the virtual desktop that it */
  /* started at */
  if(x < edge_thickness)
    *delta_x = -HorWarpSize;
  else if (x >= Scr.MyDisplayWidth-edge_thickness)
    *delta_x = HorWarpSize;
  else
    *delta_x = 0;
  if (Scr.VxMax == 0) *delta_x = 0;
  if(y < edge_thickness)
    *delta_y = -VertWarpSize;
  else if (y >= Scr.MyDisplayHeight-edge_thickness)
    *delta_y = VertWarpSize;
  else
    *delta_y = 0;
  if (Scr.VyMax == 0) *delta_y = 0;

  /* Ouch! lots of bounds checking */
  if(Scr.Vx + *delta_x < 0)
    {
      if (!(Scr.flags.edge_wrap_x))
	{
	  *delta_x = -Scr.Vx;
	  *xl = x - *delta_x;
	}
      else
	{
	  *delta_x += Scr.VxMax + Scr.MyDisplayWidth;
	  *xl = x + *delta_x % Scr.MyDisplayWidth + HorWarpSize;
	}
    }
  else if(Scr.Vx + *delta_x > Scr.VxMax)
    {
      if (!(Scr.flags.edge_wrap_x))
	{
	  *delta_x = Scr.VxMax - Scr.Vx;
	  *xl = x - *delta_x;
	}
      else
	{
	  *delta_x -= Scr.VxMax +Scr.MyDisplayWidth;
	  *xl = x + *delta_x % Scr.MyDisplayWidth - HorWarpSize;
	}
    }
  else
    *xl = x - *delta_x;

  if(Scr.Vy + *delta_y < 0)
    {
      if (!(Scr.flags.edge_wrap_y))
	{
	  *delta_y = -Scr.Vy;
	  *yt = y - *delta_y;
	}
      else
	{
	  *delta_y += Scr.VyMax + Scr.MyDisplayHeight;
	  *yt = y + *delta_y % Scr.MyDisplayHeight + VertWarpSize;
	}
    }
  else if(Scr.Vy + *delta_y > Scr.VyMax)
    {
      if (!(Scr.flags.edge_wrap_y))
	{
	  *delta_y = Scr.VyMax - Scr.Vy;
	  *yt = y - *delta_y;
	}
      else
	{
	  *delta_y -= Scr.VyMax + Scr.MyDisplayHeight;
	  *yt = y + *delta_y % Scr.MyDisplayHeight - VertWarpSize;
	}
    }
  else
    *yt = y - *delta_y;

  /* make sure the pointer isn't warped into the panframes */
  if(*xl < edge_thickness) *xl = edge_thickness;
  if(*yt < edge_thickness) *yt = edge_thickness;
  if(*xl >= Scr.MyDisplayWidth - edge_thickness)
    *xl = Scr.MyDisplayWidth - edge_thickness -1;
  if(*yt >= Scr.MyDisplayHeight - edge_thickness)
    *yt = Scr.MyDisplayHeight - edge_thickness -1;

  if ((*delta_x == 0)&&(*delta_y == 0))
    {
      my_timestamp = 0;
      add_time = 0;
      return False;
    }

  if(Grab)
    MyXGrabServer(dpy);
  /* Turn off the rubberband if its on */
  MoveOutline(Scr.Root,0,0,0,0);
  XWarpPointer(dpy,None,Scr.Root,0,0,0,0,*xl,*yt);
  MoveViewport(Scr.Vx + *delta_x,Scr.Vy + *delta_y,False);
  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
                xl, yt, &JunkX, &JunkY, &JunkMask);
  if(Grab)
    MyXUngrabServer(dpy);

  my_timestamp = 0;
  add_time = 0;
  return True;
}


/* the root window is surrounded by four window slices, which are InputOnly.
 * So you can see 'through' them, but they eat the input. An EnterEvent in
 * one of these windows causes a Paging. The windows have the according cursor
 * pointing in the pan direction or are hidden if there is no more panning
 * in that direction. This is mostly intended to get a panning even atop
 * of Motif applictions, which does not work yet. It seems Motif windows
 * eat all mouse events.
 *
 * Hermann Dunkel, HEDU, dunkel@cul-ipn.uni-kiel.de 1/94
*/

/***************************************************************************
 * checkPanFrames hides PanFrames if they are on the very border of the
 * VIRTUAL screen and EdgeWrap for that direction is off.
 * (A special cursor for the EdgeWrap border could be nice) HEDU
 ****************************************************************************/
void checkPanFrames(void)
{
  Bool wrapX = (Scr.flags.edge_wrap_x) && Scr.VxMax;
  Bool wrapY = (Scr.flags.edge_wrap_x) && Scr.VyMax;

  if(!(Scr.flags.windows_captured))
    return;

  /* thickness of 0 means remove the pan frames */
  if (edge_thickness == 0) {
    if (Scr.PanFrameTop.isMapped) {
      XUnmapWindow(dpy,Scr.PanFrameTop.win);
      Scr.PanFrameTop.isMapped=False;
    }
    if (Scr.PanFrameBottom.isMapped) {
      XUnmapWindow (dpy,Scr.PanFrameBottom.win);
      Scr.PanFrameBottom.isMapped=False;
    }
    if (Scr.PanFrameLeft.isMapped) {
      XUnmapWindow(dpy,Scr.PanFrameLeft.win);
      Scr.PanFrameLeft.isMapped=False;
    }
    if (Scr.PanFrameRight.isMapped) {
      XUnmapWindow (dpy,Scr.PanFrameRight.win);
      Scr.PanFrameRight.isMapped=False;
    }
    return;
  }

  /* check they are the right size */
  if (edge_thickness != last_edge_thickness) {
    XResizeWindow (dpy, Scr.PanFrameTop.win, Scr.MyDisplayWidth,
		   edge_thickness);
    XResizeWindow (dpy, Scr.PanFrameLeft.win, edge_thickness,
		   Scr.MyDisplayHeight);
    XMoveResizeWindow (dpy, Scr.PanFrameRight.win,
                       Scr.MyDisplayWidth - edge_thickness, 0,
                       edge_thickness, Scr.MyDisplayHeight);
    XMoveResizeWindow (dpy, Scr.PanFrameBottom.win,
                       0, Scr.MyDisplayHeight - edge_thickness,
                       Scr.MyDisplayWidth, edge_thickness);
    last_edge_thickness = edge_thickness;
  }

  /* Remove Pan frames if paging by edge-scroll is permanently or
   * temporarily disabled */
  if(Scr.EdgeScrollY == 0)
    {
      XUnmapWindow(dpy,Scr.PanFrameTop.win);
      Scr.PanFrameTop.isMapped=False;
      XUnmapWindow (dpy,Scr.PanFrameBottom.win);
      Scr.PanFrameBottom.isMapped=False;
    }
  if(Scr.EdgeScrollX == 0)
    {
      XUnmapWindow(dpy,Scr.PanFrameLeft.win);
      Scr.PanFrameLeft.isMapped=False;
      XUnmapWindow (dpy,Scr.PanFrameRight.win);
      Scr.PanFrameRight.isMapped=False;
    }
  if((Scr.EdgeScrollX == 0)&&(Scr.EdgeScrollY == 0))
    return;

  /* LEFT, hide only if EdgeWrap is off */
  if (Scr.Vx==0 && Scr.PanFrameLeft.isMapped && (!wrapX))
    {
      XUnmapWindow(dpy,Scr.PanFrameLeft.win);
      Scr.PanFrameLeft.isMapped=False;
    }
  else if ((Scr.Vx > 0 || wrapX) && Scr.PanFrameLeft.isMapped==False)
    {
      XMapRaised(dpy,Scr.PanFrameLeft.win);
      Scr.PanFrameLeft.isMapped=True;
    }
  /* RIGHT, hide only if EdgeWrap is off */
  if (Scr.Vx == Scr.VxMax && Scr.PanFrameRight.isMapped && (!wrapX))
    {
      XUnmapWindow (dpy,Scr.PanFrameRight.win);
      Scr.PanFrameRight.isMapped=False;
    }
  else if ((Scr.Vx < Scr.VxMax || wrapX) && Scr.PanFrameRight.isMapped==False)
    {
      XMapRaised(dpy,Scr.PanFrameRight.win);
      Scr.PanFrameRight.isMapped=True;
    }
  /* TOP, hide only if EdgeWrap is off */
  if (Scr.Vy==0 && Scr.PanFrameTop.isMapped && (!wrapY))
    {
      XUnmapWindow(dpy,Scr.PanFrameTop.win);
      Scr.PanFrameTop.isMapped=False;
    }
  else if ((Scr.Vy > 0 || wrapY) && Scr.PanFrameTop.isMapped==False)
    {
      XMapRaised(dpy,Scr.PanFrameTop.win);
      Scr.PanFrameTop.isMapped=True;
    }
  /* BOTTOM, hide only if EdgeWrap is off */
  if (Scr.Vy == Scr.VyMax && Scr.PanFrameBottom.isMapped && (!wrapY))
    {
      XUnmapWindow (dpy,Scr.PanFrameBottom.win);
      Scr.PanFrameBottom.isMapped=False;
    }
  else if ((Scr.Vy < Scr.VyMax || wrapY) && Scr.PanFrameBottom.isMapped==False)
    {
      XMapRaised(dpy,Scr.PanFrameBottom.win);
      Scr.PanFrameBottom.isMapped=True;
    }
}

/****************************************************************************
 *
 * Gotta make sure these things are on top of everything else, or they
 * don't work!
 *
 * For some reason, this seems to be unneeded.
 *
 ***************************************************************************/
void raisePanFrames(void)
{
  if (Scr.PanFrameTop.isMapped) XRaiseWindow(dpy,Scr.PanFrameTop.win);
  if (Scr.PanFrameLeft.isMapped) XRaiseWindow(dpy,Scr.PanFrameLeft.win);
  if (Scr.PanFrameRight.isMapped) XRaiseWindow(dpy,Scr.PanFrameRight.win);
  if (Scr.PanFrameBottom.isMapped) XRaiseWindow(dpy,Scr.PanFrameBottom.win);
}

/****************************************************************************
 *
 * Creates the windows for edge-scrolling
 *
 ****************************************************************************/
void initPanFrames(void)
{
  XSetWindowAttributes attributes;    /* attributes for create */
  unsigned long valuemask;
  int saved_thickness;

  /* Not creating the frames disables all subsequent behavior */
  /* TKP. This is bad, it will cause an XMap request on a null window later*/
  /* if (edge_thickness == 0) return; */
  saved_thickness = edge_thickness;
  if (edge_thickness == 0) edge_thickness = 2;

  attributes.event_mask =  (EnterWindowMask | LeaveWindowMask |
			    VisibilityChangeMask);
  valuemask=  (CWEventMask | CWCursor );

  attributes.cursor = Scr.FvwmCursors[TOP_EDGE];
  /* I know these overlap, it's useful when at (0,0) and the top one is unmapped */
  Scr.PanFrameTop.win =
    XCreateWindow (dpy, Scr.Root,
		   0, 0,
		   Scr.MyDisplayWidth, edge_thickness,
		   0,	/* no border */
		   CopyFromParent, InputOnly,
		   CopyFromParent,
		   valuemask, &attributes);
  attributes.cursor = Scr.FvwmCursors[LEFT_EDGE];
  Scr.PanFrameLeft.win =
    XCreateWindow (dpy, Scr.Root,
		   0, 0,
		   edge_thickness, Scr.MyDisplayHeight,
		   0,	/* no border */
		   CopyFromParent, InputOnly, CopyFromParent,
		   valuemask, &attributes);
  attributes.cursor = Scr.FvwmCursors[RIGHT_EDGE];
  Scr.PanFrameRight.win =
    XCreateWindow (dpy, Scr.Root,
		   Scr.MyDisplayWidth - edge_thickness, 0,
		   edge_thickness, Scr.MyDisplayHeight,
		   0,	/* no border */
		   CopyFromParent, InputOnly, CopyFromParent,
		   valuemask, &attributes);
  attributes.cursor = Scr.FvwmCursors[BOTTOM_EDGE];
  Scr.PanFrameBottom.win =
    XCreateWindow (dpy, Scr.Root,
		   0, Scr.MyDisplayHeight - edge_thickness,
		   Scr.MyDisplayWidth, edge_thickness,
		   0,	/* no border */
		   CopyFromParent, InputOnly, CopyFromParent,
		   valuemask, &attributes);
  Scr.PanFrameTop.isMapped=Scr.PanFrameLeft.isMapped=
    Scr.PanFrameRight.isMapped= Scr.PanFrameBottom.isMapped=False;

  edge_thickness = saved_thickness;
}


/***************************************************************************
 *
 *  Moves the viewport within the virtual desktop
 *
 ***************************************************************************/
void MoveViewport(int newx, int newy, Bool grab)
{
  FvwmWindow *t, *t1;
  int deltax,deltay;
  int PageTop, PageLeft;
  int PageBottom, PageRight;
  int txl, txr, tyt, tyb;

  if(grab)
    MyXGrabServer(dpy);

  prev_page_x = Scr.Vx;
  prev_page_y = Scr.Vy;

  if(newx > Scr.VxMax)
    newx = Scr.VxMax;
  if(newy > Scr.VyMax)
    newy = Scr.VyMax;
  if(newx <0)
    newx = 0;
  if(newy <0)
    newy = 0;

  deltay = Scr.Vy - newy;
  deltax = Scr.Vx - newx;
  /*
      Identify the bounding rectangle that will be moved into
      the viewport.
  */
  PageBottom    =  Scr.MyDisplayHeight - deltay;
  PageRight     =  Scr.MyDisplayWidth  - deltax;
  PageTop       =  0 - deltay;
  PageLeft      =  0 - deltax;

  Scr.Vx = newx;
  Scr.Vy = newy;
  BroadcastPacket(M_NEW_PAGE, 5,
                  Scr.Vx, Scr.Vy, Scr.CurrentDesk, Scr.VxMax, Scr.VyMax);

  if((deltax!=0)||(deltay!=0))
    {

/*
    RBW - 11/13/1998  - new:  chase the chain bidirectionally, all at once!
    The idea is to move the windows that are moving out of the viewport from
    the bottom of the stacking order up, to minimize the expose-redraw overhead.
    Windows that will be moving into view will be moved top down, for the same
    reason. Use the new  stacking-order chain, rather than the old
    last-focussed chain.
*/

      t = Scr.FvwmRoot.stack_next;
      t1 = Scr.FvwmRoot.stack_prev;
    while (t != &Scr.FvwmRoot || t1 != &Scr.FvwmRoot)
      {
        if (t != &Scr.FvwmRoot)
          {
	    /*
	        If the window is moving into the viewport...
	    */
            txl = t->frame_x;
            tyt = t->frame_y;
	    txr = t->frame_x + t->frame_width;
	    tyb = t->frame_y + t->frame_height;
	    if ((txr >= PageLeft && txl <= PageRight
	        && tyb >= PageTop && tyt <= PageBottom)
	        && !IS_VIEWPORT_MOVED(t)
		&& !IS_WINDOW_BEING_MOVED_OPAQUE(t))
	      {
		SET_VIEWPORT_MOVED(t, 1); /*  Block double move.  */
	        /* If the window is iconified, and sticky Icons is set,
	         * then the window should essentially be sticky */
	        if(!((IS_ICONIFIED(t))&&(IS_ICON_STICKY(t))) &&
	           (!(IS_STICKY(t))))
	          {
                    if(!(IS_ICON_STICKY(t)))
	             {
	               t->icon_x_loc += deltax;
		       t->icon_xl_loc += deltax;
		       t->icon_y_loc += deltay;
		       if(t->icon_pixmap_w != None)
		         XMoveWindow(dpy,t->icon_pixmap_w,t->icon_x_loc,
			        t->icon_y_loc);
                       if(t->icon_w != None)
                         XMoveWindow(dpy,t->icon_w,t->icon_x_loc,
                                t->icon_y_loc+t->icon_p_height);
		       if(!(IS_ICON_UNMAPPED(t)))
                        {
		         BroadcastPacket(M_ICON_LOCATION, 7,
                                         t->w, t->frame,
                                         (unsigned long)t,
                                         t->icon_x_loc, t->icon_y_loc,
                                         t->icon_w_width,
                                         t->icon_w_height+t->icon_p_height);
                        }
		     }
	           SetupFrame (t, t->frame_x+ deltax, t->frame_y + deltay,
		          t->frame_width, t->frame_height,FALSE, False);
	         }
	      }
            /*  Bump to next win...    */
            t = t->stack_next;
          }
        if (t1 != &Scr.FvwmRoot)
          {
	    /*
	        If the window is not moving into the viewport...
	    */
            txl = t1->frame_x;
            tyt = t1->frame_y;
            txr = t1->frame_x + t1->frame_width;
            tyb = t1->frame_y + t1->frame_height;
            if (! (txr >= PageLeft && txl <= PageRight
                && tyb >= PageTop && tyt <= PageBottom)
                && !IS_VIEWPORT_MOVED(t1)
		&& !IS_WINDOW_BEING_MOVED_OPAQUE(t1))
	      {
		SET_VIEWPORT_MOVED(t1, 1); /* Block double move.*/
		/* If the window is iconified, and sticky Icons is set,
		 * then the window should essentially be sticky */
		if(!(IS_ICONIFIED(t1) && IS_ICON_STICKY(t1)) &&
		   (!IS_STICKY(t1)))
		  {
		    if(!IS_ICON_STICKY(t1))
		      {
			t1->icon_x_loc += deltax;
			t1->icon_xl_loc += deltax;
			t1->icon_y_loc += deltay;
			if(t1->icon_pixmap_w != None)
			  XMoveWindow(dpy,t1->icon_pixmap_w,
				      t1->icon_x_loc,
				      t1->icon_y_loc);
			if(t1->icon_w != None)
			  XMoveWindow(dpy,t1->icon_w,t1->icon_x_loc,
				      t1->icon_y_loc+t1->icon_p_height);
			if(!IS_ICON_UNMAPPED(t1))
			  {
			    BroadcastPacket(M_ICON_LOCATION, 7,
					    t1->w, t1->frame,
					    (unsigned long)t1,
					    t1->icon_x_loc, t1->icon_y_loc,
					    t1->icon_w_width,
					    t1->icon_w_height +
					    t1->icon_p_height);
			  }
		      }
		    SetupFrame (t1, t1->frame_x+ deltax,
				t1->frame_y + deltay, t1->frame_width,
				t1->frame_height,FALSE, False);
		  }
	      }
            /*  Bump to next win...    */
            t1 = t1->stack_prev;
          }
      }
      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
	  SET_VIEWPORT_MOVED(t, 0); /* Clear double move blocker. */
	  /* If its an icon, and its sticking, autoplace it so
	   * that it doesn't wind up on top a a stationary
	   * icon */
	  if((IS_STICKY(t) || IS_ICON_STICKY(t)) &&
	     IS_ICONIFIED(t) && !IS_ICON_MOVED(t) &&
	     !IS_ICON_UNMAPPED(t))
	    AutoPlaceIcon(t);
	}

    }
  checkPanFrames();

  /* do this with PanFrames too ??? HEDU */
  while(XCheckTypedEvent(dpy,MotionNotify,&Event))
    StashEventTime(&Event);
  if(grab)
    MyXUngrabServer(dpy);

#ifdef GNOME
  /* update GNOME pager */
  GNOME_SetCurrentArea();
#endif
}

/**************************************************************************
 *
 * Move to a new desktop
 *
 *************************************************************************/
void changeDesks_func(F_CMD_ARGS)
{
  changeDesks(GetDeskNumber(action));
}

void changeDesks(int desk)
{
  int oldDesk;
  FvwmWindow *FocusWin = 0, *t, *t1;
  static FvwmWindow *StickyWin = 0;

  oldDesk = Scr.CurrentDesk;
  Scr.CurrentDesk = desk;
  if(Scr.CurrentDesk == oldDesk)
    return;
  BroadcastPacket(M_NEW_DESK, 1, Scr.CurrentDesk);
  /* Scan the window list, mapping windows on the new Desk,
   * unmapping windows on the old Desk */
  MyXGrabServer(dpy);

/*
    RBW - 11/13/1998  - new:  chase the chain bidirectionally, unmapping
    windows bottom-up and mapping them top-down, to minimize expose-redraw
    overhead. Use the new  stacking-order chain, rather than the old
    last-focussed chain.
*/
  t = Scr.FvwmRoot.stack_next;
  t1 = Scr.FvwmRoot.stack_prev;
  while (t != &Scr.FvwmRoot || t1 != &Scr.FvwmRoot)
    {
      if (t != &Scr.FvwmRoot)
        {
              if(!(IS_ICONIFIED(t) && IS_ICON_STICKY(t)) &&
		 !IS_STICKY(t) && !IS_ICON_UNMAPPED(t))
                 {
	          if(t->Desk == Scr.CurrentDesk)
	           {
	            MapIt(t);
	            if (t->FocusDesk == Scr.CurrentDesk)
	             {
		      FocusWin = t;
		     }
	           }
	         }
              else
	        /*
		    Only need to do these in one of the passes...
		*/
	        {
	         /* Window is sticky */
	         t->Desk = Scr.CurrentDesk;
	         if (Scr.Focus == t)
	          {
	           t->FocusDesk =oldDesk;
	           StickyWin = t;
	          }
	        }
          t = t->stack_next;
        }
      if (t1 != &Scr.FvwmRoot)
        {
             /* Only change mapping for non-sticky windows */
             if(!(IS_ICONIFIED(t1) && IS_ICON_STICKY(t1)) &&
	        !(IS_STICKY(t1)) && !IS_ICON_UNMAPPED(t1))
	      {
	       if(t1->Desk == oldDesk)
	        {
	         if (Scr.Focus == t1)
		   t1->FocusDesk = oldDesk;
	         else
		   t1->FocusDesk = -1;
	         UnmapIt(t1);
	        }
	      }
          t1 = t1->stack_prev;
        }
    }


  MyXUngrabServer(dpy);
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      /* If its an icon, and its sticking, autoplace it so
       * that it doesn't wind up on top a a stationary
       * icon */
      if((IS_STICKY(t) || IS_ICON_STICKY(t)) &&
	 IS_ICONIFIED(t) && !IS_ICON_MOVED(t) &&
	 !IS_ICON_UNMAPPED(t))
	AutoPlaceIcon(t);
    }

  if((FocusWin)&&(HAS_CLICK_FOCUS(FocusWin)))
#ifndef NO_REMEMBER_FOCUS
    SetFocus(FocusWin->w, FocusWin,0);
  /* OK, someone beat me up, but I don't like this. If you are a predominantly
   * focus-follows-mouse person, but put in one sticky click-to-focus window
   * (typically because you don't really want to give focus to this window),
   * then the following lines are screwed up. */
/*  else if (StickyWin && (StickyWin->flags && STICKY))
    SetFocus(StickyWin->w, StickyWin,1);*/
  else if ((FocusWin) && (!HAS_NEVER_FOCUS(FocusWin)))
#endif
    SetFocus(Scr.NoFocusWin,NULL,1);
}



/**************************************************************************
 *
 * Move a window to a new desktop
 *
 *************************************************************************/
void do_move_window_to_desk(FvwmWindow *tmp_win, int desk)
{
  if(tmp_win == NULL)
    return;

  /*
    Set the window's desktop, and map or unmap it as needed.
  */
  /* Only change mapping for non-sticky windows */
  if(!( IS_ICONIFIED(tmp_win) && IS_ICON_STICKY(tmp_win)) &&
     !IS_STICKY(tmp_win) && !IS_ICON_UNMAPPED(tmp_win))
    {
      if(tmp_win->Desk == Scr.CurrentDesk)
	{
	  tmp_win->Desk = desk;
	  UnmapIt(tmp_win);
	}
      else if(desk == Scr.CurrentDesk)
	{
	  tmp_win->Desk = desk;
	  /* If its an icon, auto-place it */
	  if(IS_ICONIFIED(tmp_win))
	    AutoPlaceIcon(tmp_win);
	  MapIt(tmp_win);
	}
      else
	tmp_win->Desk = desk;
      BroadcastConfig(M_CONFIGURE_WINDOW,tmp_win);
    }
}

/* function with parsing of command line */
void move_window_to_desk(F_CMD_ARGS)
{
  int desk;

  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;
  desk = GetDeskNumber(action);
  if(desk == tmp_win->Desk)
    return;
  do_move_window_to_desk(tmp_win, desk);
}


void scroll(F_CMD_ARGS)
{
  int x,y;
  int val1, val2, val1_unit, val2_unit;

  if (GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit) != 2)
    /* to few parameters */
    return;

  if((val1 > -100000)&&(val1 < 100000))
    x = Scr.Vx + val1*val1_unit/100;
  else
    x = Scr.Vx + (val1/1000)*val1_unit/100;

  if((val2 > -100000)&&(val2 < 100000))
    y=Scr.Vy + val2*val2_unit/100;
  else
    y = Scr.Vy + (val2/1000)*val2_unit/100;

  if(((val1 <= -100000)||(val1 >= 100000))&&(x>Scr.VxMax))
    {
      int xpixels = (Scr.VxMax / Scr.MyDisplayWidth + 1) * Scr.MyDisplayWidth;
      x %= xpixels;
      y += Scr.MyDisplayHeight * (1+((x-Scr.VxMax-1)/xpixels));
      if(y > Scr.VyMax)
        y %= (Scr.VyMax / Scr.MyDisplayHeight + 1) * Scr.MyDisplayHeight;
    }
  if(((val1 <= -100000)||(val1 >= 100000))&&(x<0))
    {
      x = Scr.VxMax;
      y -= Scr.MyDisplayHeight;
      if(y < 0)
        y=Scr.VyMax;
    }
  if(((val2 <= -100000)||(val2>= 100000))&&(y>Scr.VyMax))
    {
      int ypixels = (Scr.VyMax / Scr.MyDisplayHeight + 1) *
	Scr.MyDisplayHeight;
      y %= ypixels;
      x += Scr.MyDisplayWidth * (1+((y-Scr.VyMax-1)/ypixels));
      if(x > Scr.VxMax)
        x %= (Scr.VxMax / Scr.MyDisplayWidth + 1) * Scr.MyDisplayWidth;
    }
  if(((val2 <= -100000)||(val2>= 100000))&&(y<0))
    {
      y = Scr.VyMax;
      x -= Scr.MyDisplayWidth;
      if(x < 0)
        x=Scr.VxMax;
    }
  MoveViewport(x,y,True);
}

Bool get_page_arguments(char *action, int *page_x, int *page_y)
{
  int val[2];
  int suffix[2];
  char *token;
  char *taction;
  Bool xwrap = False;
  Bool ywrap = False;

  taction = GetNextToken(action, &token);
  if (token == NULL)
    {
      *page_x = Scr.Vx;
      *page_y = Scr.Vy;
      return True;
    }
  if (StrEquals(token, "prev"))
    {
      /* last page selected */
      *page_x = prev_page_x;
      *page_y = prev_page_y;
      free(token);
      return True;
    }
  if (StrEquals(token, "xwrap"))
    {
      xwrap = True;
    }
  else if (StrEquals(token, "ywrap"))
    {
      ywrap = True;
    }
  else if (StrEquals(token, "xywrap"))
    {
      xwrap = True;
      ywrap = True;
    }
  free(token);
  if (xwrap || ywrap)
    {
      GetNextToken(taction, &token);
      if (token == NULL)
	{
	  *page_x = Scr.Vx;
	  *page_y = Scr.Vy;
	  return True;
	}
      action = taction;
      free(token);
    }

  if (GetSuffixedIntegerArguments(action, NULL, val, 2, "p", suffix) != 2)
    return False;

  if (val[0] >= 0 || suffix[0] == 1)
    *page_x = val[0] * Scr.MyDisplayWidth;
  else
    *page_x = (val[0]+1) * Scr.MyDisplayWidth + Scr.VxMax;
  if (val[1] >= 0 || suffix[1] == 1)
    *page_y = val[1] * Scr.MyDisplayHeight;
  else
    *page_y = (val[1]+1) * Scr.MyDisplayHeight + Scr.VyMax;
  /* handle 'p' suffix */
  if (suffix[0] == 1)
    *page_x += Scr.Vx;
  if (suffix[1] == 1)
    *page_y += Scr.Vy;

  /* limit to desktop size */
  if (*page_x < 0)
    *page_x = 0;
  if (*page_y < 0)
    *page_y = 0;
  if (*page_x > Scr.VxMax)
    *page_x = Scr.VxMax;
  if (*page_y > Scr.VyMax)
    *page_y = Scr.VyMax;

  return True;
}

void goto_page_func(F_CMD_ARGS)
{
  int x;
  int y;

  if (!get_page_arguments(action, &x, &y))
    {
      fvwm_msg(ERR,"goto_page_func","GotoPage: invalid arguments: %s", action);
      return;
    }

  MoveViewport(x,y,True);
}
