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

#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "events.h"
#include "misc.h"
#include "screen.h"
#include "defaults.h"
#include "virtual.h"
#include "module_interface.h"
#include "focus.h"
#include "gnome.h"
#include "move_resize.h"
#include "borders.h"
#include "icons.h"
#include "stack.h"


/*
 * dje 12/19/98
 *
 * Testing with edge_thickness of 1 showed that some XServers don't
 * care for 1 pixel windows, causing EdgeScrolling  not to work with some
 * servers.  One  bug report was for SUNOS  4.1.3_U1.  We didn't find out
 * the exact  cause of the problem.  Perhaps  no enter/leave  events were
 * generated.
 *
 * Allowed: 0,1,or 2 pixel pan frames.
 *
 * 0   completely disables mouse  edge scrolling,  even	 while dragging a
 * window.
 *
 * 1 gives the	smallest pan frames,  which seem to  work best	except on
 * some servers.
 *
 * 2 is the default.
 */
static int edge_thickness = 2;
static int last_edge_thickness = 2;
static unsigned int prev_page_x = 0;
static unsigned int prev_page_y = 0;
static int prev_desk = 0;
static int prev_desk_and_page_desk = 0;
static unsigned int prev_desk_and_page_page_x = 0;
static unsigned int prev_desk_and_page_page_y = 0;

/**************************************************************************
 *
 * Parse arguments for "Desk" and "MoveToDesk" (formerly "WindowsDesk"):
 *
 * (nil)       : desk number = current desk
 * n	       : desk number = current desk + n
 * 0 n	       : desk number = n
 * n x	       : desk number = current desk + n
 * 0 n min max : desk number = n, but limit to min/max
 * n min max   : desk number = current desk + n, but wrap around at desk #min
 *		 or desk #max
 * n x min max : desk number = current desk + n, but wrap around at desk #min
 *		 or desk #max
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

  if (MatchToken(action, "prev"))
  {
    return prev_desk;
  }

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


/**************************************************************************
 *
 * Unmaps a window on transition to a new desktop
 *
 *************************************************************************/
static void unmap_window(FvwmWindow *t)
{
  XWindowAttributes winattrs;
  unsigned long eventMask = 0;
  Status ret;

  /*
   * Prevent the receipt of an UnmapNotify, since that would
   * cause a transition to the Withdrawn state.
   */
  ret = XGetWindowAttributes(dpy, t->w, &winattrs);
  if (ret)
  {
    eventMask = winattrs.your_event_mask;
    /* suppress UnmapRequest event */
    XSelectInput(dpy, t->w, eventMask & ~StructureNotifyMask);
  }
  if (IS_ICONIFIED(t))
  {
    if (t->icon_pixmap_w != None)
      XUnmapWindow(dpy,t->icon_pixmap_w);
    if (t->icon_w != None)
      XUnmapWindow(dpy,t->icon_w);
  }
  else
  {
    XUnmapWindow(dpy,t->frame);
#ifdef ICCCM2_UNMAP_WINDOW_PATCH
    /* this is required by the ICCCM2 */
    XUnmapWindow(dpy, t->w);
#endif
    SetMapStateProp(t, IconicState);
  }
  if (ret)
  {
    XSelectInput(dpy, t->w, eventMask);
  }

  return;
}

/**************************************************************************
 *
 * Maps a window on transition to a new desktop
 *
 *************************************************************************/
static void map_window(FvwmWindow *t)
{
  XWindowAttributes winattrs;
  unsigned long eventMask = 0;
  Status ret;

  if (IS_SCHEDULED_FOR_DESTROY(t))
  {
    return;
  }
  /*
   * Prevent the receipt of an UnmapNotify, since that would
   * cause a transition to the Withdrawn state.
   */
  ret = XGetWindowAttributes(dpy, t->w, &winattrs);
  if (ret)
  {
    eventMask = winattrs.your_event_mask;
    /* suppress MapRequest event */
    XSelectInput(dpy, t->w, eventMask & ~StructureNotifyMask);
  }
  if(IS_ICONIFIED(t))
  {
    if(t->icon_pixmap_w != None)
      XMapWindow(dpy,t->icon_pixmap_w);
    if(t->icon_w != None)
      XMapWindow(dpy,t->icon_w);
  }
  else if(IS_MAPPED(t))
  {
    XMapWindow(dpy, t->frame);
    XMapWindow(dpy, t->Parent);
    XMapWindow(dpy, t->decor_w);
#ifdef ICCCM2_UNMAP_WINDOW_PATCH
    /* this is required by the ICCCM2 */
    XMapWindow(dpy, t->w);
#endif
    SetMapStateProp(t, NormalState);
  }
  if (ret)
  {
    XSelectInput(dpy, t->w, eventMask);
  }

  return;
}


void CMD_EdgeThickness(F_CMD_ARGS)
{
  int val, n;

  n = GetIntegerArguments(action, NULL, &val, 1);
  if(n != 1) {
    fvwm_msg(ERR,"setEdgeThickness",
	     "EdgeThickness requires 1 numeric argument, found %d args",n);
    return;
  }
  if (val < 0 || val > 2) {		/* check range */
    fvwm_msg(ERR,"setEdgeThickness",
	     "EdgeThickness arg must be between 0 and 2, found %d",val);
    return;
  }
  edge_thickness = val;
  checkPanFrames();
}

void CMD_EdgeScroll(F_CMD_ARGS)
{
  int val1, val2, val1_unit,val2_unit,n;

  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
  if(n != 2)
  {
    fvwm_msg(ERR,"SetEdgeScroll","EdgeScroll requires two arguments");
    return;
  }

  /*
   * if edgescroll >1000 and < 100000m
   * wrap at edges of desktop (a "spherical" desktop)
   */
  if (val1 >= 1000)
  {
    val1 /= 1000;
    Scr.flags.edge_wrap_x = 1;
  }
  else
  {
    Scr.flags.edge_wrap_x = 0;
  }
  if (val2 >= 1000)
  {
    val2 /= 1000;
    Scr.flags.edge_wrap_y = 1;
  }
  else
  {
    Scr.flags.edge_wrap_y = 0;
  }

  Scr.EdgeScrollX = val1*val1_unit/100;
  Scr.EdgeScrollY = val2*val2_unit/100;

  checkPanFrames();
}

void CMD_EdgeResistance(F_CMD_ARGS)
{
  int val[3];
  int n;

  n = GetIntegerArguments(action, NULL, val, 3);
  if (n < 2 || n > 3)
  {
    fvwm_msg(ERR, "SetEdgeResistance",
	     "EdgeResistance requires two or three arguments");
    return;
  }
  Scr.ScrollResistance = val[0];
  Scr.MoveResistance = val[1];
  Scr.XiMoveResistance = (n < 3) ? val[1] : val[2];
}

void CMD_Xinerama(F_CMD_ARGS)
{
  int toggle;

  toggle = ParseToggleArgument(action, NULL, -1, 0);
  if (toggle == -1)
  {
    toggle = !FScreenIsEnabled();
  }
  if (!toggle != !FScreenIsEnabled())
  {
    Scr.flags.do_need_window_update = True;
    Scr.flags.has_xinerama_state_changed = True;
    FScreenOnOff(toggle);
    broadcast_xinerama_state();
  }

  return;
}

void CMD_XineramaPrimaryScreen(F_CMD_ARGS)
{
  int val;

  val = FScreenGetScreenArgument(action, 0);
  FScreenSetPrimaryScreen(val);
  if (FScreenIsEnabled())
  {
    Scr.flags.do_need_window_update = True;
    Scr.flags.has_xinerama_state_changed = True;
  }
  broadcast_xinerama_state();

  return;
}

void CMD_XineramaSls(F_CMD_ARGS)
{
  int toggle;

  toggle = ParseToggleArgument(action, NULL, -1, 0);
  if (toggle == -1)
  {
    toggle = !FScreenIsSLSEnabled();
  }
  if (!toggle != !FScreenIsSLSEnabled())
  {
    if (FScreenIsEnabled())
    {
      Scr.flags.do_need_window_update = True;
      Scr.flags.has_xinerama_state_changed = True;
    }
    FScreenSLSOnOff(toggle);
    broadcast_xinerama_state();
  }

  return;
}

void CMD_XineramaSlsSize(F_CMD_ARGS)
{
  int val[2];

  if (GetIntegerArguments(action, NULL, val, 2) != 2 &&
      GetRectangleArguments(action, &val[0], &val[1]) != 2)
  {
    val[0] = 1;
    val[1] = 1;
  }
  if (FScreenConfigureSLS(val[0], val[1]))
  {
    broadcast_xinerama_state();
  }

  return;
}


void CMD_DesktopSize(F_CMD_ARGS)
{
  int val[2];

  if (GetIntegerArguments(action, NULL, val, 2) != 2 &&
      GetRectangleArguments(action, &val[0], &val[1]) != 2)
  {
    fvwm_msg(ERR,"CMD_DesktopSize","DesktopSize requires two arguments");
    return;
  }

  Scr.VxMax = (val[0] <= 0)? 0: val[0]*Scr.MyDisplayWidth-Scr.MyDisplayWidth;
  Scr.VyMax = (val[1] <= 0)? 0: val[1]*Scr.MyDisplayHeight-Scr.MyDisplayHeight;
  BroadcastPacket(M_NEW_PAGE, 5,
		  Scr.Vx, Scr.Vy, Scr.CurrentDesk, Scr.VxMax, Scr.VyMax);

  checkPanFrames();
  /* update GNOME pager */
  GNOME_SetAreaCount();
}


/***************************************************************************
 *
 * Check to see if the pointer is on the edge of the screen, and scroll/page
 * if needed
 ***************************************************************************/
Bool HandlePaging(int HorWarpSize, int VertWarpSize, int *xl, int *yt,
		  int *delta_x, int *delta_y, Bool Grab, Bool fLoop,
		  Bool do_continue_previous)
{
  static unsigned int add_time = 0;
  int x,y;
  XEvent e;
  static Time my_timestamp = 0;
  static Time my_last_timestamp = 0;
  static Bool is_timestamp_valid = False;
  static int last_x = 0;
  static int last_y = 0;
  static Bool is_last_position_valid = False;

  *delta_x = 0;
  *delta_y = 0;

  /* Only continue to count the time, but do not start counting from
   * scratch. */
  if (!is_timestamp_valid && do_continue_previous)
  {
    /* Free some CPU */
    usleep(10000);
    return False;
  }

  if (Scr.ScrollResistance >= 10000 || (HorWarpSize == 0 && VertWarpSize==0))
  {
    is_timestamp_valid = False;
    add_time = 0;
    return False;
  }

  if (Scr.VxMax == 0 && Scr.VyMax == 0)
  {
    is_timestamp_valid = False;
    add_time = 0;
    return False;
  }

  if (!is_timestamp_valid)
  {
    is_timestamp_valid = True;
    my_timestamp = lastTimestamp;
    is_last_position_valid = False;
    add_time = 0;
    last_x = -1;
    last_y = -1;
  }
  else if (my_last_timestamp != lastTimestamp)
  {
    add_time = 0;
  }
  my_last_timestamp = lastTimestamp;

  do
  {
    if (XPending(dpy) > 0 &&
       (XCheckWindowEvent(
	 dpy, Scr.PanFrameTop.win, LeaveWindowMask, &Event) ||
	XCheckWindowEvent(
	  dpy, Scr.PanFrameBottom.win, LeaveWindowMask, &Event) ||
	XCheckWindowEvent(
	  dpy, Scr.PanFrameLeft.win, LeaveWindowMask, &Event) ||
	XCheckWindowEvent(
	  dpy, Scr.PanFrameRight.win, LeaveWindowMask, &Event)))
    {
      StashEventTime(&Event);
      is_timestamp_valid = False;
      add_time = 0;
      return False;
    }
    else if (XCheckMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask, &e))
    {
      XPutBackEvent(dpy, &e);
      is_timestamp_valid = False;
      add_time = 0;
      return False;
    }
    /* get pointer location */
    GetLocationFromEventOrQuery(dpy, Scr.Root, &Event, &x, &y);
    /* check actual pointer location since PanFrames can get buried under
       a window being moved or resized - mab */
    if (x >= edge_thickness && x < Scr.MyDisplayWidth-edge_thickness &&
        y >= edge_thickness && y < Scr.MyDisplayHeight-edge_thickness)
    {
      is_timestamp_valid = False;
      add_time = 0;
      discard_window_events(Scr.PanFrameTop.win, LeaveWindowMask);
      discard_window_events(Scr.PanFrameBottom.win, LeaveWindowMask);
      discard_window_events(Scr.PanFrameLeft.win, LeaveWindowMask);
      discard_window_events(Scr.PanFrameRight.win, LeaveWindowMask);
      return False;
    }
    if (!fLoop && is_last_position_valid &&
	(x - last_x > MAX_PAGING_MOVE_DISTANCE ||
	 x - last_x < -MAX_PAGING_MOVE_DISTANCE ||
	 y - last_y > MAX_PAGING_MOVE_DISTANCE ||
	 y - last_y < -MAX_PAGING_MOVE_DISTANCE))
    {
      /* The pointer is moving too fast, prevent paging until it slows
       * down. Don't prevent paging when fLoop is set since we can't be
       * sure that HandlePaging will be called again. */
      is_timestamp_valid = True;
      my_timestamp = lastTimestamp;
      add_time = 0;
      last_x = x;
      last_y = y;
      return False;
    }
    last_x = x;
    last_y = y;
    is_last_position_valid = True;
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
  if (*xl < edge_thickness)
    *xl = edge_thickness;
  if (*yt < edge_thickness)
    *yt = edge_thickness;
  if (*xl >= Scr.MyDisplayWidth - edge_thickness)
    *xl = Scr.MyDisplayWidth - edge_thickness -1;
  if (*yt >= Scr.MyDisplayHeight - edge_thickness)
    *yt = Scr.MyDisplayHeight - edge_thickness -1;

  is_timestamp_valid = False;
  add_time = 0;
  if (*delta_x == 0 && *delta_y == 0)
  {
    return False;
  }

  if(Grab)
    MyXGrabServer(dpy);
  /* Turn off the rubberband if its on */
  switch_move_resize_grid(False);
  XWarpPointer(dpy,None,Scr.Root,0,0,0,0,*xl,*yt);
  MoveViewport(Scr.Vx + *delta_x,Scr.Vy + *delta_y,False);
  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		xl, yt, &JunkX, &JunkY, &JunkMask);
  if(Grab)
    MyXUngrabServer(dpy);

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
  Bool do_unmap_l = False;
  Bool do_unmap_r = False;
  Bool do_unmap_t = False;
  Bool do_unmap_b = False;

  if (!Scr.flags.windows_captured)
    return;

  /* thickness of 0 means remove the pan frames */
  if (edge_thickness == 0)
  {
    do_unmap_l = True;
    do_unmap_r = True;
    do_unmap_t = True;
    do_unmap_b = True;
  }
  /* Remove Pan frames if paging by edge-scroll is permanently or
   * temporarily disabled */
  if (Scr.EdgeScrollX == 0 || Scr.VxMax == 0)
  {
    do_unmap_l = True;
    do_unmap_r = True;
  }
  if (Scr.EdgeScrollY == 0 || Scr.VyMax == 0)
  {
    do_unmap_t = True;
    do_unmap_b = True;
  }
  if (Scr.Vx == 0 && !Scr.flags.edge_wrap_x)
  {
    do_unmap_l = True;
  }
  if (Scr.Vx == Scr.VxMax && !Scr.flags.edge_wrap_x)
  {
    do_unmap_r = True;
  }
  if (Scr.Vy == 0 && !Scr.flags.edge_wrap_y)
  {
    do_unmap_t = True;
  }
  if (Scr.Vy == Scr.VyMax && !Scr.flags.edge_wrap_y)
  {
    do_unmap_b = True;
  }

  /*
   * hide or show the windows
   */

  /* left */
  if (do_unmap_l)
  {
    if (Scr.PanFrameLeft.isMapped)
    {
      XUnmapWindow(dpy, Scr.PanFrameLeft.win);
      Scr.PanFrameLeft.isMapped = False;
    }
  }
  else
  {
    if (edge_thickness != last_edge_thickness)
    {
      XResizeWindow(
	dpy, Scr.PanFrameLeft.win, edge_thickness, Scr.MyDisplayHeight);
    }
    if (!Scr.PanFrameLeft.isMapped)
    {
      XMapRaised(dpy, Scr.PanFrameLeft.win);
      Scr.PanFrameLeft.isMapped = True;
    }
  }
  /* right */
  if (do_unmap_r)
  {
    if (Scr.PanFrameRight.isMapped)
    {
      XUnmapWindow(dpy, Scr.PanFrameRight.win);
      Scr.PanFrameRight.isMapped = False;
    }
  }
  else
  {
    if (edge_thickness != last_edge_thickness)
    {
      XMoveResizeWindow(
	dpy, Scr.PanFrameRight.win, Scr.MyDisplayWidth - edge_thickness, 0,
	edge_thickness, Scr.MyDisplayHeight);
    }
    if (!Scr.PanFrameRight.isMapped)
    {
      XMapRaised(dpy, Scr.PanFrameRight.win);
      Scr.PanFrameRight.isMapped = True;
    }
  }
  /* top */
  if (do_unmap_t)
  {
    if (Scr.PanFrameTop.isMapped)
    {
      XUnmapWindow(dpy, Scr.PanFrameTop.win);
      Scr.PanFrameTop.isMapped = False;
    }
  }
  else
  {
    if (edge_thickness != last_edge_thickness)
    {
      XResizeWindow(
	dpy, Scr.PanFrameTop.win, Scr.MyDisplayWidth, edge_thickness);
    }
    if (!Scr.PanFrameTop.isMapped)
    {
      XMapRaised(dpy, Scr.PanFrameTop.win);
      Scr.PanFrameTop.isMapped = True;
    }
  }
  /* bottom */
  if (do_unmap_b)
  {
    if (Scr.PanFrameBottom.isMapped)
    {
      XUnmapWindow(dpy, Scr.PanFrameBottom.win);
      Scr.PanFrameBottom.isMapped = False;
    }
  }
  else
  {
    if (edge_thickness != last_edge_thickness)
    {
      XMoveResizeWindow(
	dpy, Scr.PanFrameBottom.win, 0, Scr.MyDisplayHeight - edge_thickness,
	Scr.MyDisplayWidth, edge_thickness);
    }
    if (!Scr.PanFrameBottom.isMapped)
    {
      XMapRaised(dpy, Scr.PanFrameBottom.win);
      Scr.PanFrameBottom.isMapped = True;
    }
  }
  last_edge_thickness = edge_thickness;

  return;
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
  if (Scr.PanFrameTop.isMapped)
    XRaiseWindow(dpy,Scr.PanFrameTop.win);
  if (Scr.PanFrameLeft.isMapped)
    XRaiseWindow(dpy,Scr.PanFrameLeft.win);
  if (Scr.PanFrameRight.isMapped)
    XRaiseWindow(dpy,Scr.PanFrameRight.win);
  if (Scr.PanFrameBottom.isMapped)
    XRaiseWindow(dpy,Scr.PanFrameBottom.win);
}

/****************************************************************************
 *
 * Creates the windows for edge-scrolling
 *
 ****************************************************************************/
void initPanFrames(void)
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;
  int saved_thickness;

  /* Not creating the frames disables all subsequent behavior */
  /* TKP. This is bad, it will cause an XMap request on a null window later*/
  /* if (edge_thickness == 0) return; */
  saved_thickness = edge_thickness;
  if (edge_thickness == 0)
    edge_thickness = 2;

  attributes.event_mask = XEVMASK_PANFW;
  valuemask=  (CWEventMask | CWCursor);

  attributes.cursor = Scr.FvwmCursors[CRS_TOP_EDGE];
  /* I know these overlap, it's useful when at (0,0) and the top one is
   * unmapped */
  Scr.PanFrameTop.win =
    XCreateWindow (dpy, Scr.Root,
		   0, 0,
		   Scr.MyDisplayWidth, edge_thickness,
		   0,	/* no border */
		   CopyFromParent, InputOnly,
		   CopyFromParent,
		   valuemask, &attributes);
  attributes.cursor = Scr.FvwmCursors[CRS_LEFT_EDGE];
  Scr.PanFrameLeft.win =
    XCreateWindow (dpy, Scr.Root,
		   0, 0,
		   edge_thickness, Scr.MyDisplayHeight,
		   0,	/* no border */
		   CopyFromParent, InputOnly, CopyFromParent,
		   valuemask, &attributes);
  attributes.cursor = Scr.FvwmCursors[CRS_RIGHT_EDGE];
  Scr.PanFrameRight.win =
    XCreateWindow (dpy, Scr.Root,
		   Scr.MyDisplayWidth - edge_thickness, 0,
		   edge_thickness, Scr.MyDisplayHeight,
		   0,	/* no border */
		   CopyFromParent, InputOnly, CopyFromParent,
		   valuemask, &attributes);
  attributes.cursor = Scr.FvwmCursors[CRS_BOTTOM_EDGE];
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
  FvwmWindow *sf = get_focus_window();

  if (grab)
    MyXGrabServer(dpy);
  if (newx > Scr.VxMax)
    newx = Scr.VxMax;
  if (newy > Scr.VyMax)
    newy = Scr.VyMax;
  if (newx <0)
    newx = 0;
  if (newy <0)
    newy = 0;
  deltay = Scr.Vy - newy;
  deltax = Scr.Vx - newx;
  /*
    Identify the bounding rectangle that will be moved into
    the viewport.
  */
  PageBottom    =  Scr.MyDisplayHeight - deltay - 1;
  PageRight     =  Scr.MyDisplayWidth  - deltax - 1;
  PageTop       =  0 - deltay;
  PageLeft      =  0 - deltax;
  if (deltax || deltay)
  {
    prev_page_x = Scr.Vx;
    prev_page_y = Scr.Vy;
    prev_desk_and_page_page_x = Scr.Vx;
    prev_desk_and_page_page_y = Scr.Vy;
    prev_desk_and_page_desk = Scr.CurrentDesk;
  }
  Scr.Vx = newx;
  Scr.Vy = newy;
  BroadcastPacket(
    M_NEW_PAGE, 5, Scr.Vx, Scr.Vy, Scr.CurrentDesk, Scr.VxMax, Scr.VyMax);

  if (deltax || deltay)
  {

    /*
     * RBW - 11/13/1998	 - new:	 chase the chain bidirectionally, all at once!
     * The idea is to move the windows that are moving out of the viewport from
     * the bottom of the stacking order up, to minimize the expose-redraw
     * overhead. Windows that will be moving into view will be moved top down,
     * for the same reason. Use the new	 stacking-order chain, rather than the
     * old last-focussed chain.
     *
     * domivogt (29-Nov-1999): It's faster to first map windows top to bottom
     * and then unmap windows bottom up.
     */
    t = get_next_window_in_stack_ring(&Scr.FvwmRoot);
    while (t != &Scr.FvwmRoot)
    {
      /*
       * If the window is moving into the viewport...
       */
      txl = t->frame_g.x;
      tyt = t->frame_g.y;
      txr = t->frame_g.x + t->frame_g.width - 1;
      tyb = t->frame_g.y + t->frame_g.height - 1;
      if ((IS_STICKY(t) || (IS_ICONIFIED(t) && IS_ICON_STICKY(t))) &&
          !IS_VIEWPORT_MOVED(t))
      {
	/* the absolute position has changed */
	t->normal_g.x -= deltax;
	t->normal_g.y -= deltay;
	t->max_g.x -= deltax;
	t->max_g.y -= deltay;
	SET_VIEWPORT_MOVED(t, 1); /*  Block double move.  */
      }
      if ((txr >= PageLeft && txl <= PageRight
	   && tyb >= PageTop && tyt <= PageBottom)
	  && !IS_VIEWPORT_MOVED(t)
	  && !IS_WINDOW_BEING_MOVED_OPAQUE(t))
      {
	SET_VIEWPORT_MOVED(t, 1); /*  Block double move.  */
	/* If the window is iconified, and sticky Icons is set,
	 * then the window should essentially be sticky */
	if(!(IS_ICONIFIED(t) && IS_ICON_STICKY(t)) && !IS_STICKY(t))
	{
	  if (IS_ICONIFIED(t))
	  {
	    t->icon_g.x += deltax;
	    t->icon_xl_loc += deltax;
	    t->icon_g.y += deltay;
	    if(t->icon_pixmap_w != None)
	      XMoveWindow(dpy,t->icon_pixmap_w,t->icon_g.x,
			  t->icon_g.y);
	    if(t->icon_w != None)
	      XMoveWindow(dpy,t->icon_w,t->icon_g.x,
			  t->icon_g.y+t->icon_p_height);
	    if(!(IS_ICON_UNMAPPED(t)))
	    {
	      BroadcastPacket(M_ICON_LOCATION, 7,
			      t->w, t->frame,
			      (unsigned long)t,
			      t->icon_g.x, t->icon_g.y,
			      t->icon_p_width,
			      t->icon_g.height+t->icon_p_height);
	    }
	  }
	  SetupFrame(t, t->frame_g.x+ deltax, t->frame_g.y + deltay,
		     t->frame_g.width, t->frame_g.height, False);
	}
      }
      /*  Bump to next win...	 */
      t = get_next_window_in_stack_ring(t);
    }
    t1 = get_prev_window_in_stack_ring(&Scr.FvwmRoot);
    while (t1 != &Scr.FvwmRoot)
    {
      /*
       *If the window is not moving into the viewport...
       */
      SET_VIEWPORT_MOVED(t, 1);
      txl = t1->frame_g.x;
      tyt = t1->frame_g.y;
      txr = t1->frame_g.x + t1->frame_g.width - 1;
      tyb = t1->frame_g.y + t1->frame_g.height - 1;
      if (! (txr >= PageLeft && txl <= PageRight
	     && tyb >= PageTop && tyt <= PageBottom)
	  && !IS_VIEWPORT_MOVED(t1)
	  && !IS_WINDOW_BEING_MOVED_OPAQUE(t1))
      {
	/* If the window is iconified, and sticky Icons is set,
	 * then the window should essentially be sticky */
	if (!(IS_ICONIFIED(t1) && IS_ICON_STICKY(t1)) && !IS_STICKY(t1))
	{
	  if (IS_ICONIFIED(t1))
	  {
	    t1->icon_g.x += deltax;
	    t1->icon_xl_loc += deltax;
	    t1->icon_g.y += deltay;
	    if(t1->icon_pixmap_w != None)
	      XMoveWindow(dpy,t1->icon_pixmap_w,
			  t1->icon_g.x,
			  t1->icon_g.y);
	    if(t1->icon_w != None)
	      XMoveWindow(dpy,t1->icon_w,t1->icon_g.x,
			  t1->icon_g.y+t1->icon_p_height);
	    if(!IS_ICON_UNMAPPED(t1))
	    {
	      BroadcastPacket(M_ICON_LOCATION, 7,
			      t1->w, t1->frame,
			      (unsigned long)t1,
			      t1->icon_g.x, t1->icon_g.y,
			      t1->icon_p_width,
			      t1->icon_g.height +
			      t1->icon_p_height);
	    }
	  }
	  SetupFrame(t1, t1->frame_g.x+ deltax,
		     t1->frame_g.y + deltay, t1->frame_g.width,
		     t1->frame_g.height, False);
	}
      }
      /*  Bump to next win...	 */
      t1 = get_prev_window_in_stack_ring(t1);
    }
    for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if (IS_VIEWPORT_MOVED(t))
      {
	SET_VIEWPORT_MOVED(t, 0); /* Clear double move blocker. */
	GNOME_SetWinArea(t);
      }
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
  if ((sf = get_focus_window()))
  {
    /* regrab buttons for focused window in case it is now obscured */
    focus_grab_buttons(sf, True);
  }

  /* do this with PanFrames too ??? HEDU */
  while(XCheckTypedEvent(dpy,MotionNotify,&Event))
    StashEventTime(&Event);
  if(grab)
    MyXUngrabServer(dpy);
  /* update GNOME pager */
  GNOME_SetCurrentArea();
}

/************************************************************************
 *
 * Unmap all windows on a desk -
 *   - Part 1 of a desktop switch
 *   - must eventually be followed by a call to MapDesk
 *   - unmaps from the bottom of the stack up
 *
 *************************************************************************/
static void UnmapDesk(int desk, Bool grab)
{
  FvwmWindow *t;
  FvwmWindow *sf = get_focus_window();

  if (grab)
  {
    MyXGrabServer(dpy);
  }
  for (t = get_prev_window_in_stack_ring(&Scr.FvwmRoot); t != &Scr.FvwmRoot;
       t = get_prev_window_in_stack_ring(t))
  {
    /* Only change mapping for non-sticky windows */
    if(!(IS_ICONIFIED(t) && IS_ICON_STICKY(t)) &&
       !(IS_STICKY(t)) && !IS_ICON_UNMAPPED(t))
    {
      if (t->Desk == desk)
      {
	if (sf == t)
	{
	  t->flags.is_focused_on_other_desk = 1;
	  t->FocusDesk = desk;
	  DeleteFocus(1);
	}
	else
	{
	  t->flags.is_focused_on_other_desk = 0;
	}
	unmap_window(t);
      }
    }
    else
    {
      t->flags.is_focused_on_other_desk = 0;
    }
  }
  if (grab)
  {
    MyXUngrabServer(dpy);
  }

  return;
}


/************************************************************************
 *
 * Map all windows on a desk -
 *   - Part 2 of a desktop switch
 *   - only use if UnmapDesk has previously been called
 *   - maps from the top of the stack down
 *
 ************************************************************************/
static void MapDesk(int desk, Bool grab)
{
  FvwmWindow *t;
  FvwmWindow *FocusWin = NULL;
  FvwmWindow *StickyWin = NULL;
  FvwmWindow *sf = get_focus_window();

  Scr.flags.is_map_desk_in_progress = 1;
  if (grab)
  {
    MyXGrabServer(dpy);
  }
  for (t = get_next_window_in_stack_ring(&Scr.FvwmRoot); t != &Scr.FvwmRoot;
       t = get_next_window_in_stack_ring(t))
  {
    /* Only change mapping for non-sticky windows */
    if(!(IS_ICONIFIED(t) && IS_ICON_STICKY(t)) &&
       !(IS_STICKY(t)) && !IS_ICON_UNMAPPED(t))
    {
      if(t->Desk == desk)
      {
	map_window(t);
      }
    }
    else
    {
      /*  If window is sticky, just update its desk (it's still mapped).  */
      t->Desk = desk;
      if (sf == t)
      {
	StickyWin = t;
      }
    }
  }
  if (grab)
  {
    MyXUngrabServer(dpy);
  }

  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    /*
      Autoplace any sticky icons, so that sticky icons from the old
      desk don't land on top of stationary ones on the new desk.
    */
    if((IS_STICKY(t) || IS_ICON_STICKY(t)) &&
       IS_ICONIFIED(t) && !IS_ICON_MOVED(t) &&
       !IS_ICON_UNMAPPED(t))
    {
      AutoPlaceIcon(t);
    }
    /*	Keep track of the last-focused window on the new desk.	*/
    if (t->flags.is_focused_on_other_desk && t->FocusDesk == desk)
    {
      t->flags.is_focused_on_other_desk = 0;
      FocusWin = t;
    }
  }

  /*  If a sticky window has focus, don't disturb it.  */
  if (!StickyWin)
  {
    /*  Otherwise, handle remembering the last-focused clicky window.  */
    if (FocusWin && (HAS_CLICK_FOCUS(FocusWin) || HAS_SLOPPY_FOCUS(FocusWin)))
    {
      ReturnFocusWindow(FocusWin, 1);
    }
    else if (FocusWin && !HAS_NEVER_FOCUS(FocusWin))
    {
      DeleteFocus(1);
    }
  }
  Scr.flags.is_map_desk_in_progress = 0;

  return;
}


/**************************************************************************
 *
 * Move to a new desktop
 *
 *************************************************************************/
void CMD_GotoDesk(F_CMD_ARGS)
{
  goto_desk(GetDeskNumber(action));
}

void CMD_Desk(F_CMD_ARGS)
{
  CMD_GotoDesk(F_PASS_ARGS);
}

void goto_desk(int desk)
{
  /*
    RBW - the unmapping operations are now removed to their own functions so
    they can also be used by the new GoToDeskAndPage command.
  */
  if (Scr.CurrentDesk != desk)
  {
    prev_desk = Scr.CurrentDesk;
    prev_desk_and_page_desk = Scr.CurrentDesk;
    prev_desk_and_page_page_x = Scr.Vx;
    prev_desk_and_page_page_y = Scr.Vy;
    UnmapDesk(Scr.CurrentDesk, True);
    Scr.CurrentDesk = desk;
    MapDesk(desk, True);
    BroadcastPacket(M_NEW_DESK, 1, Scr.CurrentDesk);
    /* FIXME: domivogt (22-Apr-2000): Fake a 'restack' for sticky window
     * upon desk change.  This is a workaround for a problem in FvwmPager:
     * The pager has a separate 'root' window for each desk.  If the active
     * desk changes, the pager destroys sticky mini windows and creates new
     * ones in the other desktop 'root'.  But the pager can't know where to
     * stack them.  So we have to tell it ecplicitly where they go :-(
     * This should be fixed in the pager, but right now the pager doesn't
     * maintain the stacking order. */
    BroadcastRestackAllWindows();
    GNOME_SetCurrentDesk();
    GNOME_SetDeskCount();
  }

  return;
}



/*************************************************************************
 *
 * Move to a new desktop and page at the same time.
 *   This function is designed for use by the Pager, and replaces the old
 *   GoToDesk 0 10000 hack.
 *   - unmap all windows on the current desk so they don't flash when the
 *     viewport is moved, then switch the viewport, then the desk.
 *
 *************************************************************************/
void CMD_GotoDeskAndPage(F_CMD_ARGS)
{
  int val[3];
  Bool is_new_desk;

  if (MatchToken(action, "prev"))
  {
    val[0] = prev_desk_and_page_desk;
    val[1] = prev_desk_and_page_page_x;
    val[2] = prev_desk_and_page_page_y;
  }
  else if (GetIntegerArguments(action, NULL, val, 3) != 3)
  {
    return;
  }

  is_new_desk = (Scr.CurrentDesk != val[0]);
  if (is_new_desk)
  {
    UnmapDesk(Scr.CurrentDesk, True);
  }
  prev_desk_and_page_page_x = Scr.Vx;
  prev_desk_and_page_page_y = Scr.Vy;
  MoveViewport(val[1] * Scr.MyDisplayWidth, val[2] * Scr.MyDisplayHeight, True);
  if (is_new_desk)
  {
    prev_desk = Scr.CurrentDesk;
    prev_desk_and_page_desk = Scr.CurrentDesk;
    Scr.CurrentDesk = val[0];
    MapDesk(val[0], True);
    BroadcastPacket(M_NEW_DESK, 1, Scr.CurrentDesk);
    /* FIXME: domivogt (22-Apr-2000): Fake a 'restack' for sticky window
     * upon desk change.  This is a workaround for a problem in FvwmPager:
     * The pager has a separate 'root' window for each desk.  If the active
     * desk changes, the pager destroys sticky mini windows and creates new
     * ones in the other desktop 'root'.  But the pager can't know where to
     * stack them.  So we have to tell it ecplicitly where they go :-(
     * This should be fixed in the pager, but right now the pager doesn't
     * maintain the stacking order. */
    BroadcastRestackAllWindows();
  }
  else
  {
    BroadcastPacket(M_NEW_DESK, 1, Scr.CurrentDesk);
  }

  GNOME_SetCurrentDesk();
  GNOME_SetDeskCount();

  return;
}


void CMD_GotoPage(F_CMD_ARGS)
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


/**************************************************************************
 *
 * Move a window to a new desktop
 *
 *************************************************************************/
void do_move_window_to_desk(FvwmWindow *tmp_win, int desk)
{
  if (tmp_win == NULL)
    return;

  /*
   * Set the window's desktop, and map or unmap it as needed.
   */
  /* Only change mapping for non-sticky windows */
  if (!(IS_ICONIFIED(tmp_win) && IS_ICON_STICKY(tmp_win)) &&
      !IS_STICKY(tmp_win) /*&& !IS_ICON_UNMAPPED(tmp_win)*/)
  {
    if (tmp_win->Desk == Scr.CurrentDesk)
    {
      tmp_win->Desk = desk;
      if (tmp_win == get_focus_window())
      {
	DeleteFocus(0);
      }
      unmap_window(tmp_win);
    }
    else if(desk == Scr.CurrentDesk)
    {
      tmp_win->Desk = desk;
      /* If its an icon, auto-place it */
      if (IS_ICONIFIED(tmp_win))
	AutoPlaceIcon(tmp_win);
      map_window(tmp_win);
    }
    else
    {
      tmp_win->Desk = desk;
    }
    BroadcastConfig(M_CONFIGURE_WINDOW,tmp_win);
  }
  GNOME_SetDeskCount();
  GNOME_SetDesk(tmp_win);
  GNOME_SetWinArea(tmp_win);
}

/* function with parsing of command line */
void CMD_MoveToDesk(F_CMD_ARGS)
{
  int desk;

  if (DeferExecution(eventp,&w,&tmp_win,&context,CRS_SELECT,ButtonRelease))
    return;
  desk = GetDeskNumber(action);
  if (desk == tmp_win->Desk)
    return;
  do_move_window_to_desk(tmp_win, desk);
}


void CMD_Scroll(F_CMD_ARGS)
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
