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

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "focus.h"
#include "icons.h"
#include "borders.h"
#include "virtual.h"
#include "stack.h"

static Bool lastFocusType;

/********************************************************************
 *
 * Sets the input focus to the indicated window.
 *
 **********************************************************************/
static void DoSetFocus(Window w, FvwmWindow *Fw, Bool FocusByMouse, Bool NoWarp)
{
  int i;
  extern Time lastTimestamp;

  if (Fw && HAS_NEVER_FOCUS(Fw))
  {
    /* make sure the window is not hilighted */
    DrawDecorations(Fw, DRAW_ALL, False, False, None);
    return;
  }

  /* ClickToFocus focus queue manipulation - only performed for
   * Focus-by-mouse type focus events */
  /* Watch out: Fw may not be on the windowlist and the windowlist may be
   * empty */
  if (Fw && Fw != Scr.Focus && Fw != &Scr.FvwmRoot)
  {
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
	      /*
	          RBW - 12/09/1999 - this grab stuff looks way out of sync with
		  the regular handling - look into this later. I can't test a
		  multi-head setup just now.
	      */
	      /* Need to grab buttons for focus window */
	      XSync(dpy,0);
	      for(i=0;i<3;i++)
		if(Scr.buttons2grab & (1<<i))
		  {
		    XGrabButton(dpy,(i+1),0,Scr.Ungrabbed->Parent,True,
			      ButtonPressMask, GrabModeSync,GrabModeAsync,None,
			      Scr.FvwmCursors[CRS_SYS]);
		  }
	      Scr.Focus = NULL;
	      Scr.Ungrabbed = NULL;
	      FOCUS_SET(Scr.NoFocusWin);
	    }
	  return;
	}
    }

  if((Fw != NULL)&&
     (!IsRectangleOnThisPage(&(Fw->frame_g),Fw->Desk))&&(!NoWarp))
    {
      Fw = NULL;
      w = Scr.NoFocusWin;
    }

  /*
      RBW - 1999/12/08 - we have to re-grab the unfocused window here for the
      MouseFocusClickRaises case also, but we can't ungrab the newly focused
      window here, or we'll never catch the raise click. For this special case,
      the newly-focused window is ungrabbed in events.c (HandleButtonPress).
  */
  if ((Scr.Ungrabbed != NULL)&&
      (HAS_CLICK_FOCUS(Scr.Ungrabbed) || Scr.go.MouseFocusClickRaises) &&
      (Scr.Ungrabbed != Fw))
    {
      /* need to grab all buttons for window that we are about to
       * unfocus */
      XSync(dpy,0);
      for(i=0;i<3;i++)
	if(Scr.buttons2grab & (1<<i))
	  XGrabButton(dpy,(i+1),0,Scr.Ungrabbed->Parent,True,
		      ButtonPressMask, GrabModeSync,GrabModeAsync,None,
		      Scr.FvwmCursors[CRS_SYS]);
      Scr.Ungrabbed = NULL;
    }
  /* if we do click to focus, remove the grab on mouse events that
   * was made to detect the focus change */
  if((Fw != NULL)&&(HAS_CLICK_FOCUS(Fw)))
    {
      for(i=0;i<3;i++)
	if(Scr.buttons2grab & (1<<i))
	  {
	    XUngrabButton(dpy,(i+1),0,Fw->Parent);
	    XUngrabButton(dpy,(i+1),GetUnusedModifiers(),Fw->Parent);
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

  if(Fw && IS_LENIENT(Fw))
    {
      FOCUS_SET(w);
      Scr.Focus = Fw;
      Scr.UnknownWinFocused = None;
    }
  else if (!Fw || !(Fw->wmhints) || !(Fw->wmhints->flags & InputHint) ||
           Fw->wmhints->input != False)
    {
      /* Window will accept input focus */
      FOCUS_SET(w);
      Scr.Focus = Fw;
      Scr.UnknownWinFocused = None;
    }
  else if ((Scr.Focus)&&(Scr.Focus->Desk == Scr.CurrentDesk))
    {
      /* Window doesn't want focus. Leave focus alone */
      /* FOCUS_SET(Scr.Hilite->w);*/
    }
  else
    {
      FOCUS_SET(Scr.NoFocusWin);
      Scr.Focus = NULL;
    }


  if ((Fw)&&(WM_TAKES_FOCUS(Fw)))
  {
    send_clientmessage(dpy, w, _XA_WM_TAKE_FOCUS, lastTimestamp);
  }

  XSync(dpy,0);
}

void SetFocus(Window w, FvwmWindow *Fw, Bool FocusByMouse)
{
  DoSetFocus(w, Fw, FocusByMouse, False);
}

void SetPointerEventPosition(XEvent *eventp, int x, int y)
{
  switch (eventp->type)
  {
  case ButtonPress:
  case ButtonRelease:
  case KeyPress:
  case KeyRelease:
  case MotionNotify:
    eventp->xbutton.x_root = x;
    eventp->xbutton.y_root = y;
    break;
  default:
    break;
  }

  return;
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
      cx = t->icon_xl_loc + t->icon_g.width/2;
      cy = t->icon_g.y + t->icon_p_height + ICON_HEIGHT(t) / 2;
    }
    else
    {
      cx = t->frame_g.x + t->frame_g.width/2;
      cy = t->frame_g.y + t->frame_g.height/2;
    }

    dx = (cx + Scr.Vx)/Scr.MyDisplayWidth*Scr.MyDisplayWidth;
    dy = (cy +Scr.Vy)/Scr.MyDisplayHeight*Scr.MyDisplayHeight;

    MoveViewport(dx,dy,True);

    /* If the window is still not visible, make it visible! */
    if(((t->frame_g.x + t->frame_g.height)< 0)||
       (t->frame_g.y + t->frame_g.width < 0)||
       (t->frame_g.x >Scr.MyDisplayWidth)||(t->frame_g.y>Scr.MyDisplayHeight))
    {
      SetupFrame(t, 0, 0, t->frame_g.width, t->frame_g.height, False);
      if(HAS_MOUSE_FOCUS(t) || HAS_SLOPPY_FOCUS(t))
      {
	XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, 2,2);
      }
    }
  }

  UngrabEm(GRAB_NORMAL);
  DoSetFocus(t->w, t, FocusByMouse, NoWarp);
}

/**************************************************************************
 *
 * Moves pointer to specified window
 *
 *************************************************************************/
static void WarpOn(XEvent *eventp, FvwmWindow *t, int warp_x, int x_unit,
                   int warp_y, int y_unit)
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
    cx = t->icon_xl_loc + t->icon_g.width/2;
    cy = t->icon_g.y + t->icon_p_height + ICON_HEIGHT(t) / 2;
  }
  else
  {
    cx = t->frame_g.x + t->frame_g.width/2;
    cy = t->frame_g.y + t->frame_g.height/2;
  }

  dx = (cx + Scr.Vx) / Scr.MyDisplayWidth * Scr.MyDisplayWidth;
  dy = (cy +Scr.Vy) / Scr.MyDisplayHeight * Scr.MyDisplayHeight;

  MoveViewport(dx,dy,True);

  if(IS_ICONIFIED(t))
  {
    x = t->icon_xl_loc + t->icon_g.width/2;
    y = t->icon_g.y + t->icon_p_height + ICON_HEIGHT(t) / 2;
  }
  else
  {
    if (x_unit != Scr.MyDisplayWidth)
      x = t->frame_g.x + warp_x;
    else
      x = t->frame_g.x + (t->frame_g.width - 1) * warp_x / 100;
    if (y_unit != Scr.MyDisplayHeight)
      y = t->frame_g.y + warp_y;
    else
      y = t->frame_g.y + (t->frame_g.height - 1) * warp_y / 100;
  }
  if (warp_x >= 0 && warp_y >= 0) {
    XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, x, y);
    SetPointerEventPosition(eventp, x, y);
  }
  RaiseWindow(t);

  /* If the window is still not visible, make it visible! */
  if (t->frame_g.x + t->frame_g.width  < 0 ||
      t->frame_g.y + t->frame_g.height < 0 ||
      t->frame_g.x >= Scr.MyDisplayWidth   ||
      t->frame_g.y >= Scr.MyDisplayHeight)
  {
    SetupFrame(t, 0, 0, t->frame_g.width, t->frame_g.height, False);
    XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, 2,2);
    SetPointerEventPosition(eventp, 2, 2);
  }
}


void flip_focus_func(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context,CRS_SELECT,ButtonRelease))
    return;

  /* Reorder the window list */
  FocusOn(tmp_win, TRUE, action);
}

void focus_func(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context,CRS_SELECT,ButtonRelease))
    return;

  FocusOn(tmp_win, FALSE, action);
}

void warp_func(F_CMD_ARGS)
{
   int val1_unit, val2_unit, n;
   int val1, val2;

  if (DeferExecution(eventp,&w,&tmp_win,&context,CRS_SELECT,ButtonRelease))
    return;

   n = GetTwoArguments (action, &val1, &val2, &val1_unit, &val2_unit);

   if (n == 2)
     WarpOn (eventp, tmp_win, val1, val1_unit, val2, val2_unit);
   else
     WarpOn (eventp, tmp_win, 0, 0, 0, 0);
}

Bool IsLastFocusSetByMouse(void)
{
  return lastFocusType;
}
