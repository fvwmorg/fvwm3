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
#include "externs.h"
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
/* Last window which Fvwm gave the focus to NOT the window that really has the
 * focus */
static FvwmWindow *ScreenFocus = NULL;
/* Window which had focus before the pointer moved to a different screen. */
static FvwmWindow *LastScreenFocus = NULL;

Bool do_accept_input_focus(FvwmWindow *tmp_win)
{
  return (!tmp_win || !tmp_win->wmhints ||
	  !(tmp_win->wmhints->flags & InputHint) || tmp_win->wmhints->input);
}

/********************************************************************
 *
 * Sets the input focus to the indicated window.
 *
 **********************************************************************/
static void DoSetFocus(Window w, FvwmWindow *Fw, Bool FocusByMouse, Bool NoWarp)
{
  extern Time lastTimestamp;
  FvwmWindow *sf;

  if (Fw && WM_TAKES_FOCUS(Fw))
  {
    send_clientmessage(dpy, w, _XA_WM_TAKE_FOCUS, lastTimestamp);
  }
  if (Fw && HAS_NEVER_FOCUS(Fw))
  {
    if (WM_TAKES_FOCUS(Fw))
    {
      /* give it a chance to take the focus itself */
      XSync(dpy, 0);
    }
    else
    {
      /* make sure the window is not hilighted */
      DrawDecorations(Fw, DRAW_ALL, False, False, None);
    }
    return;
  }
  if (Fw && !IS_LENIENT(Fw) &&
      Fw->wmhints && (Fw->wmhints->flags & InputHint) && !Fw->wmhints->input &&
      (sf = get_focus_window()) && sf->Desk == Scr.CurrentDesk)
  {
    return;
  }
  /* ClickToFocus focus queue manipulation - only performed for
   * Focus-by-mouse type focus events */
  /* Watch out: Fw may not be on the windowlist and the windowlist may be
   * empty */
  if (Fw && Fw != get_focus_window() && Fw != &Scr.FvwmRoot)
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

  if (!Fw && !Scr.flags.is_pointer_on_this_screen)
  {
    focus_grab_buttons(Scr.Ungrabbed, False);
    set_focus_window(NULL);
    /* DV (25-Nov-2000): Don't give the Scr.NoFocusWin the focus here. This
     * would steal the focus from the other screen's root window again. */
    /* FOCUS_SET(Scr.NoFocusWin); */
    return;
  }

  if (Fw && !NoWarp)
  {
    if (IS_ICONIFIED(Fw))
    {
      rectangle r;

      r.x = Fw->icon_g.x;
      r.y = Fw->icon_g.y;
      r.width = Fw->icon_g.width;
      r.height = Fw->icon_p_height + Fw->icon_g.height;
      if (!IsRectangleOnThisPage(&r, Fw->Desk))
      {
        Fw = NULL;
        w = Scr.NoFocusWin;
      }
    }
    else if (!IsRectangleOnThisPage(&(Fw->frame_g),Fw->Desk))
    {
      Fw = NULL;
      w = Scr.NoFocusWin;
    }
  }

  /*
      RBW - 1999/12/08 - we have to re-grab the unfocused window here for the
      MouseFocusClickRaises case also, but we can't ungrab the newly focused
      window here, or we'll never catch the raise click. For this special case,
      the newly-focused window is ungrabbed in events.c (HandleButtonPress).
  */
  if (Scr.Ungrabbed != Fw)
  {
    /* need to grab all buttons for window that we are about to unfocus */
    focus_grab_buttons(Scr.Ungrabbed, False);
  }
  /* if we do click to focus, remove the grab on mouse events that
   * was made to detect the focus change */
  if (Fw && HAS_CLICK_FOCUS(Fw))
  {
    focus_grab_buttons(Fw, True);
  }
  /* RBW - allow focus to go to a NoIconTitle icon window so
   * auto-raise will work on it... */
  if (Fw && IS_ICONIFIED(Fw))
  {
    Bool is_window_selected = False;

    if (Fw->icon_w)
    {
      w = Fw->icon_w;
      is_window_selected = True;
    }
    if ((!is_window_selected || WAS_ICON_HINT_PROVIDED(Fw)) &&
	Fw->icon_pixmap_w)
    {
      w = Fw->icon_pixmap_w;
    }
  }

  if (Fw && IS_LENIENT(Fw))
  {
    FOCUS_SET(w);
    set_focus_window(Fw);
    SET_FOCUS_CHANGE_BROADCAST_PENDING(Fw, 1);
    Scr.UnknownWinFocused = None;
  }
  else if (!Fw || !(Fw->wmhints) || !(Fw->wmhints->flags & InputHint) ||
           Fw->wmhints->input != False)
  {
    /* Window will accept input focus */
    FOCUS_SET(w);
    set_focus_window(Fw);
    if (Fw)
    {
      SET_FOCUS_CHANGE_BROADCAST_PENDING(Fw, 1);
    }
    Scr.UnknownWinFocused = None;
  }
  else if ((sf = get_focus_window()) && sf->Desk == Scr.CurrentDesk)
  {
    /* Window doesn't want focus. Leave focus alone */
    /* FOCUS_SET(Scr.Hilite->w);*/
  }
  else
  {
    FOCUS_SET(Scr.NoFocusWin);
    set_focus_window(NULL);
  }
  XSync(dpy,0);

  return;
}

static void MoveFocus(
  Window w, FvwmWindow *Fw, Bool FocusByMouse, Bool NoWarp, Bool do_force)
{
  FvwmWindow *ffw_old = get_focus_window();
  Bool accepts_input_focus = do_accept_input_focus(Fw);
#if 0
  FvwmWindow *ffw_new;
#endif

  if (!do_force && Fw == ffw_old)
  {
    if (ffw_old)
    {
#if 0
      /*
          RBW - 2001/08/17 - For a MouseFocusClickRaises win, we must not drop
          the grab, since the (potential) click hasn't happened yet.
      */
      if ((!HAS_SLOPPY_FOCUS(ffw_old) && !HAS_MOUSE_FOCUS(ffw_old)) ||
	  !DO_RAISE_MOUSE_FOCUS_CLICK(ffw_old))
      {
        focus_grab_buttons(ffw_old, True);
      }
#else
      focus_grab_buttons(ffw_old, True);
#endif
    }
    return;
  }
  DoSetFocus(w, Fw, FocusByMouse, NoWarp);
  if (get_focus_window() != ffw_old)
  {
    if (accepts_input_focus)
    {
#if 0
      /*  RBW - Ibid.  */
      ffw_new = get_focus_window();
      if (ffw_new)
      {
	if ((!HAS_SLOPPY_FOCUS(ffw_new) && !HAS_MOUSE_FOCUS(ffw_new)) ||
	    !DO_RAISE_MOUSE_FOCUS_CLICK(ffw_new))
	{
	  focus_grab_buttons(ffw_new, True);
	}
      }
#else
      focus_grab_buttons(get_focus_window(), True);
#endif
    }
    focus_grab_buttons(ffw_old, False);
  }
}

void SetFocusWindow(FvwmWindow *Fw, Bool FocusByMouse)
{
  MoveFocus(Fw->w, Fw, FocusByMouse, False, False);
}

void ReturnFocusWindow(FvwmWindow *Fw, Bool FocusByMouse)
{
  MoveFocus(Fw->w, Fw, FocusByMouse, True, False);
}

void DeleteFocus(Bool FocusByMouse)
{
  MoveFocus(Scr.NoFocusWin, NULL, FocusByMouse, False, False);
}

void ForceDeleteFocus(Bool FocusByMouse)
{
  MoveFocus(Scr.NoFocusWin, NULL, FocusByMouse, False, True);
}

/* When a window is unmapped (or destroyed) this function takes care of
 * adjusting the focus window appropriately. */
void restore_focus_after_unmap(
  FvwmWindow *tmp_win, Bool do_skip_marked_transients)
{
  extern FvwmWindow *colormap_win;
  FvwmWindow *t = NULL;
  FvwmWindow *set_focus_to = NULL;

  if (tmp_win == get_focus_window())
  {
    if (tmp_win->transientfor != None && tmp_win->transientfor != Scr.Root)
    {
      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
      {
	if (t->w == tmp_win->transientfor &&
	    t->Desk == tmp_win->Desk &&
	    (!do_skip_marked_transients || !IS_IN_TRANSIENT_SUBTREE(t)))
	{
	  set_focus_to = t;
	  break;
	}
      }
    }
    if (!set_focus_to &&
	(HAS_CLICK_FOCUS(tmp_win) || HAS_SLOPPY_FOCUS(tmp_win)))
    {
      for (t = tmp_win->next; t != NULL; t = t->next)
      {
	if (t->Desk == tmp_win->Desk &&
	    !DO_SKIP_CIRCULATE(t) &&
	    !(DO_SKIP_ICON_CIRCULATE(t) && IS_ICONIFIED(t)) &&
	    (!do_skip_marked_transients || !IS_IN_TRANSIENT_SUBTREE(t)))
	{
	  /* If it is on a different desk we have to look for another window */
	  set_focus_to = t;
	  break;
	}
      }
    }
    if (set_focus_to && set_focus_to != tmp_win &&
	set_focus_to->Desk == tmp_win->Desk)
    {
      /* Don't transfer focus to windows on other desks */
      SetFocusWindow(set_focus_to, 1);
    }
    if (tmp_win == get_focus_window())
    {
      DeleteFocus(1);
    }
  }
  if (tmp_win == Scr.pushed_window)
    Scr.pushed_window = NULL;
  if (tmp_win == colormap_win)
    colormap_win = NULL;

  return;
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
  Bool do_not_warp;

  if (t == NULL || HAS_NEVER_FOCUS(t))
  {
    UngrabEm(GRAB_NORMAL);
    if (t)
    {
      /* give the window a chance to take the focus itself */
      MoveFocus(t->w, t, FocusByMouse, 1, 0);
    }
    return;
  }

  if (!(do_not_warp = StrEquals(PeekToken(action, NULL), "NoWarp")))
  {
    if (t->Desk != Scr.CurrentDesk)
    {
      goto_desk(t->Desk);
    }

    if (IS_ICONIFIED(t))
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
    if (((t->frame_g.x + t->frame_g.height)< 0)||
	(t->frame_g.y + t->frame_g.width < 0)||
	(t->frame_g.x >Scr.MyDisplayWidth)||(t->frame_g.y>Scr.MyDisplayHeight))
    {
      SetupFrame(t, 0, 0, t->frame_g.width, t->frame_g.height, False);
      if (HAS_MOUSE_FOCUS(t) || HAS_SLOPPY_FOCUS(t))
      {
	XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, 2,2);
      }
    }
  }

  UngrabEm(GRAB_NORMAL);
  if (t->Desk == Scr.CurrentDesk)
  {
    MoveFocus(t->w, t, FocusByMouse, do_not_warp, 0);
  }

  return;
}

/**************************************************************************
 *
 * Moves pointer to specified window
 *
 *************************************************************************/
static void warp_to_fvwm_window(
  XEvent *eventp, FvwmWindow *t, int warp_x, int x_unit, int warp_y, int y_unit)
{
  int dx,dy;
  int cx,cy;
  int x,y;

  if(t == (FvwmWindow *)0 || (IS_ICONIFIED(t) && t->icon_w == None))
    return;

  if(t->Desk != Scr.CurrentDesk)
  {
    goto_desk(t->Desk);
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
  dy = (cy + Scr.Vy) / Scr.MyDisplayHeight * Scr.MyDisplayHeight;

  MoveViewport(dx,dy,True);

  if(IS_ICONIFIED(t))
  {
    x = t->icon_xl_loc + t->icon_g.width / 2;
    y = t->icon_g.y + t->icon_p_height + ICON_HEIGHT(t) / 2;
  }
  else
  {
    if (x_unit != Scr.MyDisplayWidth && warp_x >= 0)
      x = t->frame_g.x + warp_x;
    else if (x_unit != Scr.MyDisplayWidth)
      x = t->frame_g.x + t->frame_g.width + warp_x;
    else if (warp_x >= 0)
      x = t->frame_g.x + (t->frame_g.width - 1) * warp_x / 100;
    else
      x = t->frame_g.x + (t->frame_g.width - 1) * (100 + warp_x) / 100;

    if (y_unit != Scr.MyDisplayHeight && warp_y >= 0)
      y = t->frame_g.y + warp_y;
    else if (y_unit != Scr.MyDisplayHeight)
      y = t->frame_g.y + t->frame_g.height + warp_y;
    else if (warp_y >= 0)
      y = t->frame_g.y + (t->frame_g.height - 1) * warp_y / 100;
    else
      y = t->frame_g.y + (t->frame_g.height - 1) * (100 + warp_y) / 100;
  }
  XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, x, y);
  SetPointerEventPosition(eventp, x, y);
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


void CMD_FlipFocus(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context,CRS_SELECT,ButtonRelease))
    return;

  /* Reorder the window list */
  FocusOn(tmp_win, TRUE, action);
}

void CMD_Focus(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context,CRS_SELECT,ButtonRelease))
    return;

  FocusOn(tmp_win, FALSE, action);
}

void CMD_WarpToWindow(F_CMD_ARGS)
{
  int val1_unit, val2_unit, n;
  int val1, val2;

  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
  if (context != C_UNMANAGED)
  {
    if (DeferExecution(eventp,&w,&tmp_win,&context,CRS_SELECT,ButtonRelease))
      return;
    if (n == 2)
      warp_to_fvwm_window(eventp, tmp_win, val1, val1_unit, val2, val2_unit);
    else
      warp_to_fvwm_window(eventp, tmp_win, 0, 0, 0, 0);
  }
  else
  {
    int x = 0;
    int y = 0;

    if (n == 2)
    {
      int wx;
      int wy;
      unsigned int ww;
      unsigned int wh;

      if (!XGetGeometry(
	dpy, w, &JunkRoot, &wx, &wy, &ww, &wh, &JunkBW, &JunkDepth))
      {
	return;
      }
      if (val1_unit != Scr.MyDisplayWidth)
	x = wx + val1;
      else
	x = wx + (ww - 1) * val1 / 100;

      if (val2_unit != Scr.MyDisplayHeight)
	y = wy + val2;
      else
	y = wy + (wh - 1) * val2 / 100;
    }
    XWarpPointer(dpy, None, w, 0, 0, 0, 0, x, y);
  }
}

Bool IsLastFocusSetByMouse(void)
{
  return lastFocusType;
}

void focus_grab_buttons(FvwmWindow *tmp_win, Bool is_focused)
{
  int i;
  Bool accepts_input_focus;
  Bool do_grab_window = False;
  unsigned char grab_buttons = Scr.buttons2grab;

  if (!tmp_win)
  {
    return;
  }
  accepts_input_focus = do_accept_input_focus(tmp_win);
  if ((HAS_SLOPPY_FOCUS(tmp_win) || HAS_MOUSE_FOCUS(tmp_win) ||
       HAS_NEVER_FOCUS(tmp_win)) &&
      DO_RAISE_MOUSE_FOCUS_CLICK(tmp_win) &&
      (!is_focused || !is_on_top_of_layer(tmp_win)))
  {
    grab_buttons = ((1 << NUMBER_OF_MOUSE_BUTTONS) - 1);
    do_grab_window = True;
  }
  else if (HAS_CLICK_FOCUS(tmp_win) &&
	   (!is_focused || !is_on_top_of_layer(tmp_win)) &&
	   (!DO_NOT_RAISE_CLICK_FOCUS_CLICK(tmp_win) || accepts_input_focus))
  {
    grab_buttons = ((1 << NUMBER_OF_MOUSE_BUTTONS) - 1);
    do_grab_window = True;
  }

#if 0
  /*
      RBW - If we've come here to grab and all buttons are already grabbed,
      or to ungrab and none is grabbed, then we've nothing to do.
  */
  if ((!is_focused && grab_buttons == tmp_win->grabbed_buttons) ||
     (is_focused && ((tmp_win->grabbed_buttons & grab_buttons) == 0)))
  {
    return;
  }
#else
  if (grab_buttons != tmp_win->grabbed_buttons)
#endif
  {
    Bool do_grab;

    MyXGrabServer(dpy);
    Scr.Ungrabbed = (do_grab_window) ? NULL : tmp_win;
    for (i = 0; i < NUMBER_OF_MOUSE_BUTTONS; i++)
    {
#if 0
      /*  RBW - Set flag for grab or ungrab according to how we were called. */
      if (!is_focused||1)
      {
        do_grab = !!(grab_buttons & (1 << i));
      }
      else
      {
        do_grab = !(grab_buttons & (1 << i));
      }
#else
      if ((grab_buttons & (1 << i)) == (tmp_win->grabbed_buttons & (1 << i)))
	continue;
      do_grab = !!(grab_buttons & (1 << i));
#endif

      {
	register unsigned int mods;
	register unsigned int max = GetUnusedModifiers();
	register unsigned int living_modifiers = ~max;

	/* handle all bindings for the dead modifiers */
	for (mods = 0; mods <= max; mods++)
	{
	  /* Since mods starts with 1 we don't need to test if mods
	   * contains a dead modifier. Otherwise both, dead and living
	   * modifiers would be zero ==> mods == 0 */
	  if (!(mods & living_modifiers))
	  {
	    if (do_grab)
	    {
	      XGrabButton(
		dpy, i + 1, mods, tmp_win->Parent, True, ButtonPressMask,
		GrabModeSync, GrabModeAsync, None, None);
              /*  Set each FvwmWindow flag accordingly, as we grab or ungrab. */
              tmp_win->grabbed_buttons |= (1<<i);
	    }
	    else
	    {
	      XUngrabButton(dpy, (i+1), mods, tmp_win->Parent);
              tmp_win->grabbed_buttons &= !(1<<i);
	    }
	  }
	} /* for */
      }
    } /* for */
    MyXUngrabServer (dpy);

  }

  return;
}

/* same as above, but forces to regrab buttons on the window under the pointer
 * if necessary */
void focus_grab_buttons_on_pointer_window(void)
{
  Window w;
  FvwmWindow *tmp_win;

  if (!XQueryPointer(
	dpy, Scr.Root, &JunkRoot, &w, &JunkX, &JunkY, &JunkX, &JunkY,
	&JunkMask))
  {
    /* pointer is not on this screen */
    return;
  }
  if (XFindContext(dpy, w, FvwmContext, (caddr_t *) &tmp_win) == XCNOENT)
  {
    /* pointer is not over a window */
    return;
  }
  focus_grab_buttons(tmp_win, (tmp_win == get_focus_window()));

  return;
}

/* functions to access ScreenFocus, LastScreenFocus and PreviousFocus */
FvwmWindow *get_focus_window(void)
{
  return ScreenFocus;
}

void set_focus_window(FvwmWindow *fw)
{
  ScreenFocus = fw;

  return;
}

FvwmWindow *get_last_screen_focus_window(void)
{
  return LastScreenFocus;
}

void set_last_screen_focus_window(FvwmWindow *fw)
{
  LastScreenFocus = fw;

  return;
}

void update_last_screen_focus_window(FvwmWindow *fw)
{
  if (fw == LastScreenFocus)
    LastScreenFocus = NULL;

  return;
}

void set_focus_model(FvwmWindow *fw)
{
  if (fw->wmhints && (fw->wmhints->flags & InputHint) && fw->wmhints->input)
  {
    if (WM_TAKES_FOCUS(fw))
    {
      fw->focus_model = FM_LOCALLY_ACTIVE;
    }
    else
    {
      fw->focus_model = FM_PASSIVE;
    }
  }
  else
  {
    if (WM_TAKES_FOCUS(fw))
    {
      fw->focus_model = FM_GLOBALLY_ACTIVE;
    }
    else
    {
      fw->focus_model = FM_NO_INPUT;
    }
  }

  return;
}

/* This function is part of a hack to make focus handling work better with
 * applications that use the passive focus model but manage focus in their own
 * sub windows and should thus use the locally active focus model instead.
 * There are many examples like netscape or ddd. */
void refresh_focus(FvwmWindow *fw)
{
  Bool do_refresh = False;

  if (fw == NULL || fw != get_focus_window() || HAS_NEVER_FOCUS(fw))
  {
    /* only refresh the focus on the currently focused window */
    return;
  }

  /* only refresh the focus for windows with the passive focus model so that
   * we don't disturb focus handling more than necessary */
  switch (fw->focus_model)
  {
  case FM_PASSIVE:
    do_refresh = True;
    break;
  case FM_NO_INPUT:
  case FM_GLOBALLY_ACTIVE:
  case FM_LOCALLY_ACTIVE:
  default:
    do_refresh = False;
    break;
  }
  if (do_refresh)
  {
    XWindowAttributes winattrs;

    MyXGrabServer(dpy);
    if (XGetWindowAttributes(dpy, fw->w, &winattrs))
    {
      XSelectInput(dpy, fw->w, winattrs.your_event_mask & ~FocusChangeMask);
      FOCUS_SET(fw->w);
      XSelectInput(dpy, fw->w, winattrs.your_event_mask);
    }
    MyXUngrabServer(dpy);
  }

  return;
}
