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
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/***********************************************************************
 *
 * fvwm focus-setting code
 *
 ***********************************************************************/

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "fvwm.h"
#include "focus.h"
#include "icons.h"
#include "screen.h"
#include "bindings.h"
#include "stack.h"


static Bool lastFocusType;

/********************************************************************
 *
 * Sets the input focus to the indicated window.
 *
 **********************************************************************/
static void DoSetFocus(Window w, FvwmWindow *Fw, Bool FocusByMouse,
		       Bool NoWarp)
{
  int i;
  extern Time lastTimestamp;

  if (Fw && HAS_NEVER_FOCUS(Fw))
    return;

  /* ClickToFocus focus queue manipulation - only performed for
   * Focus-by-mouse type focus events */
  /* Watch out: Fw may not be on the windowlist and the windowlist may be
   * empty */
  if (Fw && Fw != Scr.Focus && Fw != &Scr.FvwmRoot) {
    if (FocusByMouse) /* pluck window from list and deposit at top */
    {
      /* remove Fw from list */
      if (Fw->prev)
	Fw->prev->next = Fw->next;
      if (Fw->next)
	Fw->next->prev = Fw->prev;

      /* insert Fw at start */
      Fw->next = Scr.FvwmRoot.next;
      if (Scr.FvwmRoot.next)
	Scr.FvwmRoot.next->prev = Fw;
      Scr.FvwmRoot.next = Fw;
      Fw->prev = &Scr.FvwmRoot;
    }
    else
    {
      /* move the windowlist around so that Fw is at the top */
      FvwmWindow *tmp_win;

      /* find the window on the windowlist */
      tmp_win = &Scr.FvwmRoot;
      while (tmp_win && tmp_win != Fw)
        tmp_win = tmp_win->next;

      if (tmp_win) /* the window is on the (non-zero length) windowlist */
      {
        /* make tmp_win point to the last window on the list */
        while (tmp_win->next)
          tmp_win = tmp_win->next;

        /* close the ends of the windowlist */
        tmp_win->next = Scr.FvwmRoot.next;
        Scr.FvwmRoot.next->prev = tmp_win;

        /* make Fw the new start of the list */
        Scr.FvwmRoot.next = Fw;
        /* open the closed loop windowlist */
        Fw->prev->next = NULL;
        Fw->prev = &Scr.FvwmRoot;
      }
    }
  }
  lastFocusType = FocusByMouse;

  if(Scr.NumberOfScreens > 1)
    {
      XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &JunkX, &JunkY, &JunkX, &JunkY, &JunkMask);
      if(JunkRoot != Scr.Root)
	{
	  if((Scr.Ungrabbed != NULL)&&(HAS_CLICK_FOCUS(Scr.Ungrabbed)))
	    {
	      /* Need to grab buttons for focus window */
	      XSync(dpy,0);
	      for(i=0;i<3;i++)
		if(Scr.buttons2grab & (1<<i))
		  {
		    XGrabButton(dpy,(i+1),0,Scr.Ungrabbed->frame,True,
				ButtonPressMask, GrabModeSync,GrabModeAsync,
				None,Scr.FvwmCursors[SYS]);
		    XGrabButton(dpy,(i+1),GetUnusedModifiers(),
				Scr.Ungrabbed->frame,True,
				ButtonPressMask, GrabModeSync,GrabModeAsync,
				None,Scr.FvwmCursors[SYS]);
		  }
	      Scr.Focus = NULL;
	      Scr.Ungrabbed = NULL;
	      XSetInputFocus(dpy, Scr.NoFocusWin,RevertToParent,lastTimestamp);
	    }
	  return;
	}
    }

  if((Fw != NULL)&&(!IsWindowOnThisPage(Fw))&&(!NoWarp))
    {
      Fw = NULL;
      w = Scr.NoFocusWin;
    }

  if((Scr.Ungrabbed != NULL)&&(HAS_CLICK_FOCUS(Scr.Ungrabbed))
     && (Scr.Ungrabbed != Fw))
    {
      /* need to grab all buttons for window that we are about to
       * unfocus */
      XSync(dpy,0);
      for(i=0;i<3;i++)
	if(Scr.buttons2grab & (1<<i))
	  XGrabButton(dpy,(i+1),0,Scr.Ungrabbed->frame,True,
		      ButtonPressMask, GrabModeSync,GrabModeAsync,None,
		      Scr.FvwmCursors[SYS]);
      Scr.Ungrabbed = NULL;
    }
  /* if we do click to focus, remove the grab on mouse events that
   * was made to detect the focus change */
  if((Fw != NULL)&&(HAS_CLICK_FOCUS(Fw)))
    {
      for(i=0;i<3;i++)
	if(Scr.buttons2grab & (1<<i))
	  {
	    XUngrabButton(dpy,(i+1),0,Fw->frame);
	    XUngrabButton(dpy,(i+1),GetUnusedModifiers(),Fw->frame);
	  }
      Scr.Ungrabbed = Fw;
    }
/*  RBW - allow focus to go to a NoIconTitle icon window so
    auto-raise will work on it...
  if((Fw)&&(Fw->flags & ICONIFIED)&&(Fw->icon_w))
    w= Fw->icon_w;
*/
  if((Fw)&&(IS_ICONIFIED(Fw)))
    {
      if (Fw->icon_w)
        {
          w = Fw->icon_w;
        }
      else if (Fw->icon_pixmap_w)
        {
          w = Fw->icon_pixmap_w;
        }
    }

  if((Fw)&&(IS_LENIENT(Fw)))
    {
      XSetInputFocus (dpy, w, RevertToParent, lastTimestamp);
      Scr.Focus = Fw;
      Scr.UnknownWinFocused = None;
    }
  else if(!((Fw)&&(Fw->wmhints)&&(Fw->wmhints->flags & InputHint)&&
	    (Fw->wmhints->input == False)))
    {
      /* Window will accept input focus */
      XSetInputFocus (dpy, w, RevertToParent, lastTimestamp);
      Scr.Focus = Fw;
      Scr.UnknownWinFocused = None;
    }
  else if ((Scr.Focus)&&(Scr.Focus->Desk == Scr.CurrentDesk))
    {
      /* Window doesn't want focus. Leave focus alone */
      /* XSetInputFocus (dpy,Scr.Hilite->w , RevertToParent, lastTimestamp);*/
    }
  else
    {
      XSetInputFocus (dpy, Scr.NoFocusWin, RevertToParent, lastTimestamp);
      Scr.Focus = NULL;
    }


  if ((Fw)&&(WM_TAKES_FOCUS(Fw)))
    send_clientmessage(dpy, w, _XA_WM_TAKE_FOCUS, lastTimestamp);

  XSync(dpy,0);

}

void SetFocus(Window w, FvwmWindow *Fw, Bool FocusByMouse)
{
  DoSetFocus(w, Fw, FocusByMouse, False);
}

/**************************************************************************
 *
 * Moves focus to specified window
 *
 *************************************************************************/
void FocusOn(FvwmWindow *t, Bool FocusByMouse, char *action)
{
  int dx,dy;
  int cx,cy;
  Bool NoWarp;

  if(t == NULL || HAS_NEVER_FOCUS(t))
    return;

  if (!(NoWarp = StrEquals(PeekToken(action, NULL), "NoWarp")))
  {
    if(t->Desk != Scr.CurrentDesk)
    {
      changeDesks(t->Desk);
    }

    if(IS_ICONIFIED(t))
    {
      cx = t->icon_xl_loc + t->icon_w_width/2;
      cy = t->icon_y_loc + t->icon_p_height + ICON_HEIGHT/2;
    }
    else
    {
      cx = t->frame_x + t->frame_width/2;
      cy = t->frame_y + t->frame_height/2;
    }

    dx = (cx + Scr.Vx)/Scr.MyDisplayWidth*Scr.MyDisplayWidth;
    dy = (cy +Scr.Vy)/Scr.MyDisplayHeight*Scr.MyDisplayHeight;

    MoveViewport(dx,dy,True);

    /* If the window is still not visible, make it visible! */
    if(((t->frame_x + t->frame_height)< 0)||(t->frame_y + t->frame_width < 0)||
       (t->frame_x >Scr.MyDisplayWidth)||(t->frame_y>Scr.MyDisplayHeight))
    {
      SetupFrame(t,0,0,t->frame_width, t->frame_height,False,False);
      if(HAS_MOUSE_FOCUS(t) || HAS_SLOPPY_FOCUS(t))
	XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, 2,2);
    }
  }

  UngrabEm();
  DoSetFocus(t->w, t, FocusByMouse, NoWarp);
}

/**************************************************************************
 *
 * Moves pointer to specified window
 *
 *************************************************************************/
void WarpOn(FvwmWindow *t,int warp_x, int x_unit, int warp_y, int y_unit)
{
  int dx,dy;
  int cx,cy;
  int x,y;

  if(t == (FvwmWindow *)0 || (IS_ICONIFIED(t) && t->icon_w == None))
    return;

  if(t->Desk != Scr.CurrentDesk)
  {
    changeDesks(t->Desk);
  }

  if(IS_ICONIFIED(t))
  {
    cx = t->icon_xl_loc + t->icon_w_width/2;
    cy = t->icon_y_loc + t->icon_p_height + ICON_HEIGHT/2;
  }
  else
  {
    cx = t->frame_x + t->frame_width/2;
    cy = t->frame_y + t->frame_height/2;
  }

  dx = (cx + Scr.Vx)/Scr.MyDisplayWidth*Scr.MyDisplayWidth;
  dy = (cy +Scr.Vy)/Scr.MyDisplayHeight*Scr.MyDisplayHeight;

  MoveViewport(dx,dy,True);

  if(IS_ICONIFIED(t))
  {
    x = t->icon_xl_loc + t->icon_w_width/2;
    y = t->icon_y_loc + t->icon_p_height + ICON_HEIGHT/2;
  }
  else
  {
    if (x_unit != Scr.MyDisplayWidth)
      x = t->frame_x + warp_x;
    else
      x = t->frame_x + (t->frame_width - 1) * warp_x / 100;
    if (y_unit != Scr.MyDisplayHeight)
      y = t->frame_y + warp_y;
    else
      y = t->frame_y + (t->frame_height - 1) * warp_y / 100;
  }
  if (warp_x >= 0 && warp_y >= 0) {
    XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, x, y);
  }
  RaiseWindow(t);

  /* If the window is still not visible, make it visible! */
  if(((t->frame_x + t->frame_height)< 0)||(t->frame_y + t->frame_width < 0)||
     (t->frame_x >Scr.MyDisplayWidth)||(t->frame_y>Scr.MyDisplayHeight))
  {
    SetupFrame(t,0,0,t->frame_width, t->frame_height,False,False);
    XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, 2,2);
  }
  UngrabEm();
}


void flip_focus_func(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;

  /* Reorder the window list */
  FocusOn(tmp_win, TRUE, action);
}

void focus_func(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;

  FocusOn(tmp_win, FALSE, action);
}

void warp_func(F_CMD_ARGS)
{
   int val1_unit, val2_unit, n;
   int val1, val2;

  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;

   n = GetTwoArguments (action, &val1, &val2, &val1_unit, &val2_unit);

   if (n == 2)
     WarpOn (tmp_win, val1, val1_unit, val2, val2_unit);
   else
     WarpOn (tmp_win, 0, 0, 0, 0);
}

Bool IsLastFocusSetByMouse(void)
{
  return lastFocusType;
}
