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
 * This module is based on Twm, but has been siginificantly modified
 * by Rob Nation
 ****************************************************************************/
/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/


/***********************************************************************
 *
 * fvwm event handling
 *
 ***********************************************************************/

#include "config.h"

#if HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include <libs/gravity.h>
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "defaults.h"
#include "events.h"
#include "libs/Colorset.h"
#include "fvwmsignal.h"
#include "module_interface.h"
#include "session.h"
#include "borders.h"
#include "frame.h"
#include "colormaps.h"
#include "add_window.h"
#include "icccm2.h"
#include "icons.h"
#include "gnome.h"
#include "ewmh.h"
#include "update.h"
#include "style.h"
#include "stack.h"
#include "geometry.h"
#include "focus.h"
#include "move_resize.h"
#include "virtual.h"
#include "decorations.h"
#include "schedule.h"
#include "menus.h"
#ifdef HAVE_STROKE
#include "stroke.h"
#endif /* HAVE_STROKE */

#ifndef XUrgencyHint
#define XUrgencyHint            (1L << 8)
#endif

extern void StartupStuff(void);

int Context = C_NO_CONTEXT;		/* current button press context */
static int Button = 0;
FvwmWindow *ButtonWindow = NULL;	/* button press window structure */
XEvent Event;				/* the current event */
FvwmWindow *Fw = NULL;		/* the current fvwm window */

int last_event_type=0;
static Window last_event_window=0;
Time lastTimestamp = CurrentTime;	/* until Xlib does this for us */
static FvwmWindow *xcrossing_last_grab_window = NULL;


STROKE_CODE(static int send_motion;)
STROKE_CODE(static char sequence[STROKE_MAX_SEQUENCE + 1];)

Window PressedW = None;

/*
** LASTEvent is the number of X events defined - it should be defined
** in X.h (to be like 35), but since extension (eg SHAPE) events are
** numbered beyond LASTEvent, we need to use a bigger number than the
** default, so let's undefine the default and use 256 instead.
*/
#undef LASTEvent
#ifndef LASTEvent
#define LASTEvent 256
#endif /* !LASTEvent */
typedef void (*PFEH)(void);
PFEH EventHandlerJumpTable[LASTEvent];

/***********************************************************************
 *
 *  Procedure:
 *	HandleFocusIn - handles focus in events
 *
 ************************************************************************/
void HandleFocusIn(void)
{
  XEvent d;
  Window w = None;
  Window focus_w = None;
  Window focus_fw = None;
  Pixel fc = 0;
  Pixel bc = 0;
  FvwmWindow *ffw_old = get_focus_window();
  FvwmWindow *sf;
  Bool do_force_broadcast = False;
  Bool is_unmanaged_focused = False;
  static Window last_focus_w = None;
  static Window last_focus_fw = None;
  static Bool is_never_focused = True;

  DBUG("HandleFocusIn","Routine Entered");

  /* This is a hack to make the PointerKey command work */
  if (Event.xfocus.detail != NotifyPointer)
  /**/
    w= Event.xany.window;
  while (XCheckTypedEvent(dpy, FocusIn, &d))
  {
    /* dito */
    if (d.xfocus.detail != NotifyPointer)
    /**/
      w = d.xany.window;
  }
  /* dito */
  if (w == None)
  {
    return;
  }
  /**/
  if (XFindContext(dpy, w, FvwmContext, (caddr_t *) &Fw) == XCNOENT)
  {
    Fw = NULL;
  }

  if (!Fw)
  {
    if (w != Scr.NoFocusWin)
    {
      Scr.UnknownWinFocused = w;
      focus_w = w;
      is_unmanaged_focused = True;
    }
    else
    {
      DrawDecorations(Scr.Hilite, DRAW_ALL, False, True, None, CLEAR_ALL);
      if (Scr.ColormapFocus == COLORMAP_FOLLOWS_FOCUS)
      {
	if((Scr.Hilite)&&(!IS_ICONIFIED(Scr.Hilite)))
	{
	  InstallWindowColormaps(Scr.Hilite);
	}
	else
	{
	  InstallWindowColormaps(NULL);
	}
      }
    }
    /* Not very useful if no window that fvwm and its modules know about has the
     * focus. */
    fc = GetColor(DEFAULT_FORE_COLOR);
    bc = GetColor(DEFAULT_BACK_COLOR);
  }
  else if (Fw != Scr.Hilite
	   /* domivogt (16-May-2000): This check is necessary to force sending
	    * a M_FOCUS_CHANGE packet after an unmanaged window was focused.
	    * Otherwise fvwm would believe that Scr.Hilite was still focused and
	    * not send any info to the modules. */
	   || last_focus_fw == None
	   || IS_FOCUS_CHANGE_BROADCAST_PENDING(Fw))
  {
    do_force_broadcast = IS_FOCUS_CHANGE_BROADCAST_PENDING(Fw);
    SET_FOCUS_CHANGE_BROADCAST_PENDING(Fw, 0);
    if (Fw != Scr.Hilite)
    {
      DrawDecorations(Fw, DRAW_ALL, True, True, None, CLEAR_ALL);
    }
    focus_w = FW_W(Fw);
    focus_fw = FW_W_FRAME(Fw);
    fc = Fw->hicolors.fore;
    bc = Fw->hicolors.back;
    set_focus_window(Fw);
    if (Scr.ColormapFocus == COLORMAP_FOLLOWS_FOCUS)
    {
      if((Scr.Hilite)&&(!IS_ICONIFIED(Scr.Hilite)))
      {
	InstallWindowColormaps(Scr.Hilite);
      }
      else
      {
	InstallWindowColormaps(NULL);
      }
    }
  }
  else
  {
    return;
  }
  if (is_never_focused || last_focus_fw == None ||
      focus_w != last_focus_w || focus_fw != last_focus_fw ||
      do_force_broadcast)
  {
    if (!Scr.bo.FlickeringQtDialogsWorkaround || !is_unmanaged_focused)
    {
      BroadcastPacket(M_FOCUS_CHANGE, 5, focus_w, focus_fw,
		      (unsigned long)IsLastFocusSetByMouse(), fc, bc);
      EWMH_SetActiveWindow(focus_w);
    }
    last_focus_w = focus_w;
    last_focus_fw = focus_fw;
    is_never_focused = False;
  }
  if ((sf = get_focus_window()) != ffw_old)
  {
    focus_grab_buttons(sf, True);
    focus_grab_buttons(ffw_old, False);
  }
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleKeyPress - key press event handler
 *
 ************************************************************************/
void HandleKeyPress(void)
{
  char *action;
  FvwmWindow *sf;

  DBUG("HandleKeyPress","Routine Entered");

  Context = GetContext(Fw, &Event, &PressedW);
  PressedW = None;

  /* Here's a real hack - some systems have two keys with the
   * same keysym and different keycodes. This converts all
   * the cases to one keycode. */
  Event.xkey.keycode =
    XKeysymToKeycode(dpy,XKeycodeToKeysym(dpy,Event.xkey.keycode,0));

  /* Check if there is something bound to the key */
  action = CheckBinding(Scr.AllBindings, STROKE_ARG(0) Event.xkey.keycode,
			Event.xkey.state, GetUnusedModifiers(), Context,
			KEY_BINDING);
  if (action != NULL)
  {
    ButtonWindow = Fw;
    old_execute_function(NULL, action, Fw, &Event, Context, -1, 0, NULL);
    ButtonWindow = NULL;
    return;
  }

  /* if we get here, no function key was bound to the key.  Send it
   * to the client if it was in a window we know about.
   */
  sf = get_focus_window();
  if (sf && Event.xkey.window != FW_W(sf))
  {
    Event.xkey.window = FW_W(sf);
    XSendEvent(dpy, FW_W(sf), False, KeyPressMask, &Event);
  }
  else if (Fw && Event.xkey.window != FW_W(Fw))
  {
    Event.xkey.window = FW_W(Fw);
    XSendEvent(dpy, FW_W(Fw), False, KeyPressMask, &Event);
  }
}


/***********************************************************************
 *
 *  Procedure:
 *	HandlePropertyNotify - property notify event handler
 *
 ***********************************************************************/
void HandlePropertyNotify(void)
{
  Bool OnThisPage = False;
  Bool was_size_inc_set;
  Bool has_icon_changed = False;
  Bool has_icon_pixmap_hint_changed = False;
  Bool has_icon_window_hint_changed = False;
  int old_wmhints_flags;
  int old_width_inc;
  int old_height_inc;
  int old_base_width;
  int old_base_height;
  FlocaleNameString new_name;

  DBUG("HandlePropertyNotify","Routine Entered");

  if (Event.xproperty.window == Scr.Root &&
      Event.xproperty.state == PropertyNewValue &&
      (Event.xproperty.atom == _XA_XSETROOT_ID ||
       Event.xproperty.atom == _XA_XROOTPMAP_ID))
  {
    /* background change */
    /* _XA_XSETROOT_ID is used by xpmroot, xli and others (xv send no property
     *  notify?).  _XA_XROOTPMAP_ID is used by Esetroot compatible program:
     * the problem here is that with some Esetroot compatible program we get
     * the message _before_ the background change.
     * This is fixed with Esetroot 9.2 (not yet released, 2002-01-14) */
    BroadcastPropertyChange(MX_PROPERTY_CHANGE_BACKGROUND, 0, 0, "");
    return;
  }

  if (!Fw)
  {
    return;
  }
  if (XGetGeometry(dpy, FW_W(Fw), &JunkRoot, &JunkX, &JunkY,
		   &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
  {
    return;
  }

  /*
   * Make sure at least part of window is on this page
   * before giving it focus...
   */
  OnThisPage = IsRectangleOnThisPage(&(Fw->frame_g), Fw->Desk);

  switch (Event.xproperty.atom)
  {
  case XA_WM_TRANSIENT_FOR:
    flush_property_notify(XA_WM_TRANSIENT_FOR, FW_W(Fw));
    if(XGetTransientForHint(dpy, FW_W(Fw), &FW_W_TRANSIENTFOR(Fw)))
    {
      SET_TRANSIENT(Fw, 1);
      RaiseWindow(Fw);
    }
    else
    {
      SET_TRANSIENT(Fw, 0);
    }
  break;

  case XA_WM_NAME:
    flush_property_notify(XA_WM_NAME, FW_W(Fw));
    if (HAS_EWMH_WM_NAME(Fw))
      return;
    FlocaleGetNameProperty(XGetWMName, dpy, FW_W(Fw), &new_name);
    if (new_name.name == NULL)
    {
      return;
    }

    free_window_names (Fw, True, False);
    Fw->name = new_name;
    if (Fw->name.name && strlen(Fw->name.name) > MAX_WINDOW_NAME_LEN)
      (Fw->name.name)[MAX_WINDOW_NAME_LEN] = 0;

    SET_NAME_CHANGED(Fw, 1);

    if (Fw->name.name == NULL)
      Fw->name.name = NoName; /* must not happen */

    setup_visible_name(Fw, False);
    BroadcastWindowIconNames(Fw, True, False);

    /* fix the name in the title bar */
    if(!IS_ICONIFIED(Fw))
      DrawDecorations(
	Fw, DRAW_TITLE, (Scr.Hilite == Fw), True, None, CLEAR_ALL);

    EWMH_SetVisibleName(Fw, False);
    /*
     * if the icon name is NoName, set the name of the icon to be
     * the same as the window
     */
    if (!WAS_ICON_NAME_PROVIDED(Fw))
    {
      Fw->icon_name = Fw->name;
      setup_visible_name(Fw, True);
      BroadcastWindowIconNames(Fw, False, True);
      RedoIconName(Fw);
    }
    break;

  case XA_WM_ICON_NAME:
    flush_property_notify(XA_WM_ICON_NAME, FW_W(Fw));
    if (HAS_EWMH_WM_ICON_NAME(Fw))
      return;
    FlocaleGetNameProperty(XGetWMIconName, dpy, FW_W(Fw), &new_name);
    if (new_name.name == NULL)
    {
      return;
    }

    free_window_names(Fw, False, True);
    Fw->icon_name = new_name;
    if (Fw->icon_name.name && strlen(Fw->icon_name.name) >
	MAX_ICON_NAME_LEN)
    {
      /* limit to prevent hanging X server */
      (Fw->icon_name.name)[MAX_ICON_NAME_LEN] = 0;
    }

    SET_WAS_ICON_NAME_PROVIDED(Fw, 1);
    if (Fw->icon_name.name == NULL)
    {
      Fw->icon_name .name= Fw->name.name;
      SET_WAS_ICON_NAME_PROVIDED(Fw, 0);
    }
    setup_visible_name(Fw, True);

    BroadcastWindowIconNames(Fw, False, True);
    RedoIconName(Fw);
    EWMH_SetVisibleName(Fw, True);
    break;

  case XA_WM_HINTS:
    flush_property_notify(XA_WM_HINTS, FW_W(Fw));
    /* clasen@mathematik.uni-freiburg.de - 02/01/1998 - new -
       the urgency flag is an ICCCM 2.0 addition to the WM_HINTS. */
    old_wmhints_flags = 0;
    if (Fw->wmhints)
    {
      old_wmhints_flags = Fw->wmhints->flags;
      XFree ((char *) Fw->wmhints);
    }
    setup_wm_hints(Fw);
    if (Fw->wmhints == NULL)
    {
      return;
    }

    /*
     * rebuild icon if the client either provides an icon
     * pixmap or window or has reset the hints to `no icon'.
     */
    if ((Fw->wmhints->flags & IconPixmapHint) ||
	(old_wmhints_flags       & IconPixmapHint))
    {
ICON_DBG((stderr,"hpn: iph changed (%d) '%s'\n", !!(int)(Fw->wmhints->flags & IconPixmapHint), Fw->name));
      has_icon_pixmap_hint_changed = True;
    }
    if ((Fw->wmhints->flags & IconWindowHint) ||
	(old_wmhints_flags       & IconWindowHint))
    {
ICON_DBG((stderr,"hpn: iwh changed (%d) '%s'\n", !!(int)(Fw->wmhints->flags & IconWindowHint), Fw->name));
      has_icon_window_hint_changed = True;
      SET_USE_EWMH_ICON(Fw, False);
    }
    increase_icon_hint_count(Fw);
    if (has_icon_window_hint_changed || has_icon_pixmap_hint_changed)
    {
      if (ICON_OVERRIDE_MODE(Fw) == ICON_OVERRIDE)
      {
ICON_DBG((stderr,"hpn: icon override '%s'\n", Fw->name));
	has_icon_changed = False;
      }
      else if (ICON_OVERRIDE_MODE(Fw) == NO_ACTIVE_ICON_OVERRIDE)
      {
	if (has_icon_pixmap_hint_changed)
	{
	  if (WAS_ICON_HINT_PROVIDED(Fw) == ICON_HINT_MULTIPLE)
	  {
ICON_DBG((stderr,"hpn: using further iph '%s'\n", Fw->name));
	    has_icon_changed = True;
	  }
	  else  if (Fw->icon_bitmap_file == NULL ||
		    Fw->icon_bitmap_file == Scr.DefaultIcon)
	  {
ICON_DBG((stderr,"hpn: using first iph '%s'\n", Fw->name));
	    has_icon_changed = True;
	  }
	  else
	  {
	    /* ignore the first icon pixmap hint if the application did not
	     * provide it from the start */
ICON_DBG((stderr,"hpn: first iph ignored '%s'\n", Fw->name));
	    has_icon_changed = False;
	  }
	}
	else if (has_icon_window_hint_changed)
	{
	  ICON_DBG((stderr,"hpn: using iwh '%s'\n", Fw->name));
	  has_icon_changed = True;
	}
	else
	{
ICON_DBG((stderr,"hpn: iwh not changed, hint ignored '%s'\n", Fw->name));
	  has_icon_changed = False;
	}
      }
      else /* NO_ICON_OVERRIDE */
      {
ICON_DBG((stderr,"hpn: using hint '%s'\n", Fw->name));
	has_icon_changed = True;
      }

      if (USE_EWMH_ICON(Fw))
	has_icon_changed = False;

      if (has_icon_changed)
      {
ICON_DBG((stderr,"hpn: icon changed '%s'\n", Fw->name));
	/* Okay, the icon hint has changed and style options tell us to honour
	 * this change.  Now let's see if we have to use the application
	 * provided pixmap or window (if any), the icon file provided by the
	 * window's style or the default style's icon. */
	if (Fw->icon_bitmap_file == Scr.DefaultIcon)
	  Fw->icon_bitmap_file = NULL;
	if (!Fw->icon_bitmap_file &&
	    !(Fw->wmhints->flags&(IconPixmapHint|IconWindowHint)))
	{
	  Fw->icon_bitmap_file =
	    (Scr.DefaultIcon) ? Scr.DefaultIcon : NULL;
	}
	Fw->iconPixmap = (Window)NULL;
	ChangeIconPixmap(Fw);
      }
    }

    /* clasen@mathematik.uni-freiburg.de - 02/01/1998 - new -
       the urgency flag is an ICCCM 2.0 addition to the WM_HINTS.
       Treat urgency changes by calling user-settable functions.
       These could e.g. deiconify and raise the window or temporarily
       change the decor. */
    if (!(old_wmhints_flags & XUrgencyHint) &&
	(Fw->wmhints->flags & XUrgencyHint))
    {
      old_execute_function(
	NULL, "Function UrgencyFunc", Fw, &Event, C_WINDOW, -1, 0, NULL);
    }

    if ((old_wmhints_flags & XUrgencyHint) &&
	!(Fw->wmhints->flags & XUrgencyHint))
    {
      old_execute_function(
	NULL, "Function UrgencyDoneFunc", Fw, &Event, C_WINDOW, -1, 0,
	NULL);
    }
    break;
  case XA_WM_NORMAL_HINTS:
    was_size_inc_set = IS_SIZE_INC_SET(Fw);
    old_width_inc = Fw->hints.width_inc;
    old_height_inc = Fw->hints.height_inc;
    old_base_width = Fw->hints.base_width;
    old_base_height = Fw->hints.base_height;
    XSync(dpy, 0);
    GetWindowSizeHints(Fw);
    if (old_width_inc != Fw->hints.width_inc ||
	old_height_inc != Fw->hints.height_inc)
    {
      int units_w;
      int units_h;
      int wdiff;
      int hdiff;

      if (!was_size_inc_set && old_width_inc == 1 && old_height_inc == 1)
      {
	/* This is a hack for xvile.  It sets the _inc hints after it
	 * requested that the window is mapped but before it's really
	 * visible. */
	/* do nothing */
      }
      else
      {
	size_borders b;

	get_window_borders(Fw, &b);
	/* we have to resize the unmaximized window to keep the size in
	 * resize increments constant */
	units_w = Fw->normal_g.width - b.total_size.width - old_base_width;
	units_h = Fw->normal_g.height - b.total_size.height -
		old_base_height;
	units_w /= old_width_inc;
	units_h /= old_height_inc;

	/* update the 'invisible' geometry */
	wdiff = units_w * (Fw->hints.width_inc - old_width_inc) +
	  (Fw->hints.base_width - old_base_width);
	hdiff = units_h * (Fw->hints.height_inc - old_height_inc) +
	  (Fw->hints.base_height - old_base_height);
	gravity_resize(
	  Fw->hints.win_gravity, &Fw->normal_g, wdiff, hdiff);
      }
      gravity_constrain_size(
	Fw->hints.win_gravity, Fw, &Fw->normal_g, 0);
      if (!IS_MAXIMIZED(Fw))
      {
	rectangle new_g;

	get_relative_geometry(&new_g, &Fw->normal_g);
	if (IS_SHADED(Fw))
	  get_shaded_geometry(Fw, &new_g, &new_g);
	frame_setup_window(
	  Fw, new_g.x, new_g.y, new_g.width, new_g.height, False);
      }
      else
      {
	int w;
	int h;

	maximize_adjust_offset(Fw);
	/* domivogt (07-Apr-2000): as terrible hack to work around a xterm
	 * bug: when the font size is changed in a xterm, xterm simply assumes
	 * that the wm will grant its new size.  Of course this is wrong if
	 * the xterm is maximised.  To make xterm happy, we first send a
	 * ConfigureNotify with the current (maximised) geometry + 1 pixel in
	 * height, then another one with the correct old geometry.  Changing
	 * the font multiple times will cause the xterm to shrink because
	 * gravity_constrain_size doesn't know about the initially requested
	 * dimensions. */
	w = Fw->max_g.width;
	h = Fw->max_g.height;
	gravity_constrain_size(
	  Fw->hints.win_gravity, Fw, &Fw->max_g,
	  CS_UPDATE_MAX_DEFECT);
	if (w != Fw->max_g.width ||
	    h != Fw->max_g.height)
	{
	  rectangle new_g;

	  /* This is in case the size_inc changed and the old dimensions are
	   * not multiples of the new values. */
	  get_relative_geometry(&new_g, &Fw->max_g);
	  if (IS_SHADED(Fw))
	    get_shaded_geometry(Fw, &new_g, &new_g);
	  frame_setup_window(
	    Fw, new_g.x, new_g.y, new_g.width, new_g.height, False);
	}
	else
	{
	  SendConfigureNotify(
	    Fw, Fw->frame_g.x, Fw->frame_g.y,
	    Fw->frame_g.width, Fw->frame_g.height+1, 0, False);
	  XSync(dpy, 0);
	  /* free some CPU */
	  usleep(1);
	  SendConfigureNotify(
	    Fw, Fw->frame_g.x, Fw->frame_g.y,
	    Fw->frame_g.width, Fw->frame_g.height, 0, False);
	  XSync(dpy, 0);
	}
      }
      GNOME_SetWinArea(Fw);
    }
    EWMH_SetAllowedActions(Fw);
    BroadcastConfig(M_CONFIGURE_WINDOW,Fw);
    break;

  default:
    if(Event.xproperty.atom == _XA_WM_PROTOCOLS)
      FetchWmProtocols (Fw);
    else if (Event.xproperty.atom == _XA_WM_COLORMAP_WINDOWS)
    {
      FetchWmColormapWindows (Fw);	/* frees old data */
      ReInstallActiveColormap();
    }
    else if (Event.xproperty.atom == _XA_WM_STATE)
    {
      if (Fw && HAS_CLICK_FOCUS(Fw) &&
	  Fw == get_focus_window())
      {
	if (OnThisPage)
	{
	  set_focus_window(NULL);
	  SetFocusWindow(Fw, False, True);
	}
      }
    }
    else
    {
      EWMH_ProcessPropertyNotify(Fw, &Event);
    }
    break;
  }
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleClientMessage - client message event handler
 *
 ************************************************************************/
void HandleClientMessage(void)
{
  XEvent button;

  DBUG("HandleClientMessage","Routine Entered");

  /* Process GNOME and EWMH Messages */
  if (GNOME_ProcessClientMessage(Fw, &Event))
  {
    return;
  }
  else if (EWMH_ProcessClientMessage(Fw, &Event))
  {
    return;
  }

  /* handle deletion of tear out menus */
  if (Fw && IS_TEAR_OFF_MENU(Fw) && Event.xclient.format == 32 &&
      Event.xclient.data.l[0] == _XA_WM_DELETE_WINDOW)
  {
    menu_close_tear_off_menu(Fw);
    return;
  }

  if ((Event.xclient.message_type == _XA_WM_CHANGE_STATE)&&
      (Fw)&&(Event.xclient.data.l[0]==IconicState)&&
      !IS_ICONIFIED(Fw))
  {
    if (XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		      &(button.xmotion.x_root),
		      &(button.xmotion.y_root),
		      &JunkX, &JunkY, &JunkMask) == False)
    {
      /* pointer is on a different screen */
      button.xmotion.x_root = 0;
      button.xmotion.y_root = 0;
    }
    button.type = 0;
    old_execute_function(
      NULL, "Iconify", Fw, &button, C_FRAME, -1, 0, NULL);
    return;
  }

  /* FIXME: Is this safe enough ? I guess if clients behave
     according to ICCCM and send these messages only if they
     when grabbed the pointer, it is OK */
  {
    extern Atom _XA_WM_COLORMAP_NOTIFY;
    if (Event.xclient.message_type == _XA_WM_COLORMAP_NOTIFY) {
      set_client_controls_colormaps(Event.xclient.data.l[1]);
      return;
    }
  }

  /*
  ** CKH - if we get here, it was an unknown client message, so send
  ** it to the client if it was in a window we know about.  I'm not so
  ** sure this should be done or not, since every other window manager
  ** I've looked at doesn't.  But it might be handy for a free drag and
  ** drop setup being developed for Linux.
  */
  if (Fw)
  {
    if(Event.xclient.window != FW_W(Fw))
    {
      Event.xclient.window = FW_W(Fw);
      XSendEvent(dpy, FW_W(Fw), False, NoEventMask, &Event);
    }
  }
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleExpose - expose event handler
 *
 ***********************************************************************/
void HandleExpose(void)
{
	XRectangle r;
	draw_window_parts draw_parts;

#if 0
	/* This doesn't work well. Sometimes, the expose count is zero although
	 * dozens of expose events are pending.  This happens all the time
	 * during a shading animation.  Simply flush expose events
	 * unconditionally. */
	if (Event.xexpose.count != 0)
	{
		flush_accumulate_expose(Event.xexpose.window, &Event);
	}
#else
	flush_accumulate_expose(Event.xexpose.window, &Event);
#endif
	if (Fw == NULL)
	{
		return;
	}
	r.x = Event.xexpose.x;
	r.y = Event.xexpose.y;
	r.width = Event.xexpose.width;
	r.height = Event.xexpose.height;
	if (IS_TEAR_OFF_MENU(Fw) && Event.xany.window == FW_W(Fw))
	{
		/* refresh the contents of the torn out menu */
		menu_expose(&Event, NULL);
	}
	/* redraw the decorations */
	if (Event.xany.window == FW_W_TITLE(Fw))
	{
		draw_parts = DRAW_TITLE;
	}
	else if (Event.xany.window == Fw->decor_w ||
		 Event.xany.window == FW_W_FRAME(Fw))
	{
		draw_parts = DRAW_FRAME;
	}
	else
	{
		draw_parts = DRAW_BUTTONS;
	}
	if (FftSupport && Fw->title_font &&
	    Fw->title_font->fftf.fftfont != NULL &&
	    draw_parts == DRAW_TITLE)
	{
	  draw_clipped_decorations(
		Fw, draw_parts, (Scr.Hilite == Fw), True,
		Event.xany.window, NULL, CLEAR_ALL);
	  return;
	}
	draw_clipped_decorations(
		Fw, draw_parts, (Scr.Hilite == Fw), True,
		Event.xany.window, &r, CLEAR_ALL);
	return;
}



/***********************************************************************
 *
 *  Procedure:
 *	HandleDestroyNotify - DestroyNotify event handler
 *
 ***********************************************************************/
void HandleDestroyNotify(void)
{
  DBUG("HandleDestroyNotify","Routine Entered");

  destroy_window(Fw);
  EWMH_ManageKdeSysTray(Event.xdestroywindow.window, Event.type);
  EWMH_WindowDestroyed();
  GNOME_SetClientList();
}




/***********************************************************************
 *
 *  Procedure:
 *	SendConfigureNotify - inform a client window of its geometry.
 *
 *  The input (frame) geometry will be translated to client geometry
 *  before sending.
 *
 ************************************************************************/
static void fake_map_unmap_notify(FvwmWindow *tmp_win, int event_type)
{
  XEvent client_event;
  XWindowAttributes winattrs = {0};

  if (!XGetWindowAttributes(dpy, FW_W(tmp_win), &winattrs))
  {
    return;
  }
  XSelectInput(
    dpy, FW_W(tmp_win), winattrs.your_event_mask & ~StructureNotifyMask);
  client_event.type = event_type;
  client_event.xmap.display = dpy;
  client_event.xmap.event = FW_W(tmp_win);
  client_event.xmap.window = FW_W(tmp_win);
  switch (event_type)
  {
    case MapNotify:
      client_event.xmap.override_redirect = False;
      break;
    case UnmapNotify:
      client_event.xunmap.from_configure = False;
      break;
    default:
      /* not possible if called correctly */
      break;
  }
  XSendEvent(dpy, FW_W(tmp_win), False, StructureNotifyMask, &client_event);
  XSelectInput(dpy, FW_W(tmp_win), winattrs.your_event_mask);

  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleMapRequest - MapRequest event handler
 *
 ************************************************************************/
void HandleMapRequest(void)
{
  DBUG("HandleMapRequest","Routine Entered");

  if (fFvwmInStartup)
  {
    /* Just map the damn thing, decorations are added later
     * in CaptureAllWindows. */
    XMapWindow (dpy, Event.xmaprequest.window);
    return;
  }
  HandleMapRequestKeepRaised(None, NULL, False);
}
void HandleMapRequestKeepRaised(
	Window KeepRaised, FvwmWindow *ReuseWin, Bool is_menu)
{
  extern long isIconicState;
  extern Bool isIconifiedByParent;
  extern Bool PPosOverride;
  Bool is_on_this_page = False;
  Bool is_new_window = False;
  FvwmWindow *tmp;
  FvwmWindow *sf;

  Event.xany.window = Event.xmaprequest.window;

  if (ReuseWin == NULL)
  {
    if (XFindContext(dpy, Event.xany.window, FvwmContext,
		     (caddr_t *)&Fw)==XCNOENT)
    {
      Fw = NULL;
    }
    else if (IS_MAP_PENDING(Fw))
    {
      /* The window is already going to be mapped, no need to do that twice */
      return;
    }
  }
  else
  {
    Fw = ReuseWin;
  }

  if (Fw == NULL && EWMH_IsKdeSysTrayWindow(Event.xany.window))
  {
    /* This means that the window is swallowed by kicker and that
     * kicker restart or exit. As we should assume that kicker restart
     * we should return here, if not we go into trouble ... */
    return;
  }
  if (!PPosOverride)
  {
    XFlush(dpy);
  }

  /* If the window has never been mapped before ... */
  if (!Fw || (Fw && DO_REUSE_DESTROYED(Fw)))
  {
    /* Add decorations. */
    Fw = AddWindow(Event.xany.window, ReuseWin, is_menu);
    if (Fw == NULL)
      return;
    is_new_window = True;
  }
  /*
   * Make sure at least part of window is on this page
   * before giving it focus...
   */
  is_on_this_page = IsRectangleOnThisPage(&(Fw->frame_g), Fw->Desk);
  if(KeepRaised != None)
  {
    XRaiseWindow(dpy, KeepRaised);
  }
  /* If it's not merely iconified, and we have hints, use them. */

  if (IS_ICONIFIED(Fw))
  {
    /* If no hints, or currently an icon, just "deiconify" */
    DeIconify(Fw);
  }
  else if (IS_MAPPED(Fw))
  {
    /* the window is already mapped - fake a MapNotify event */
    fake_map_unmap_notify(Fw, MapNotify);
  }
  else
  {
    int state;

    if (Fw->wmhints && (Fw->wmhints->flags & StateHint))
      state = Fw->wmhints->initial_state;
    else
      state = NormalState;

    if (DO_START_ICONIC(Fw))
      state = IconicState;

    if (isIconicState != DontCareState)
      state = isIconicState;

    switch (state)
    {
    case DontCareState:
    case NormalState:
    case InactiveState:
    default:
      MyXGrabServer(dpy);
      if (Fw->Desk == Scr.CurrentDesk)
      {
	Bool do_grab_focus;

	XMapWindow(dpy, FW_W_FRAME(Fw));
	XMapWindow(dpy, FW_W(Fw));
	SET_MAP_PENDING(Fw, 1);
	SetMapStateProp(Fw, NormalState);
	if (Scr.flags.is_map_desk_in_progress)
	  do_grab_focus = False;
	else if (!is_on_this_page)
	  do_grab_focus = False;
	else if (DO_GRAB_FOCUS(Fw) &&
		 (!IS_TRANSIENT(Fw) ||
		  FW_W_TRANSIENTFOR(Fw) == Scr.Root))
	{
	  do_grab_focus = True;
	}
	else if (IS_TRANSIENT(Fw) && DO_GRAB_FOCUS_TRANSIENT(Fw) &&
		 (sf = get_focus_window()) &&
		 FW_W(sf) == FW_W_TRANSIENTFOR(Fw))
	{
	  /* it's a transient and its transientfor currently has focus. */
	  do_grab_focus = True;
	}
	else if (IS_TRANSIENT(Fw) && DO_GRAB_FOCUS(Fw) &&
		 !(XGetGeometry(
			   dpy, FW_W_TRANSIENTFOR(Fw), &JunkRoot, &JunkX,
			   &JunkY, &JunkWidth, &JunkHeight, &JunkBW,
			   &JunkDepth)))
	{
	  /* Gee, the transientfor does not exist! These evil application
	   * programmers must hate us a lot ;-) */
	  FW_W_TRANSIENTFOR(Fw) = Scr.Root;
	  do_grab_focus = True;
	}
	else
	  do_grab_focus = False;
	if (do_grab_focus)
	{
	  SetFocusWindow(Fw, True, True);
	}
	else
	{
	  /* make sure the old focused window still has grabbed all necessary
	   * buttons. */
	  if ((sf = get_focus_window()))
	  {
	    focus_grab_buttons(sf, True);
	  }
	}
      }
      else
      {
#ifndef ICCCM2_UNMAP_WINDOW_PATCH
	/* nope, this is forbidden by the ICCCM2 */
	XMapWindow(dpy, FW_W(Fw));
	SetMapStateProp(Fw, NormalState);
#else
	/* Since we will not get a MapNotify, set the IS_MAPPED flag manually.
	 */
	SET_MAPPED(Fw, 1);
	SetMapStateProp(Fw, IconicState);
	/* fake that the window was mapped to allow modules to swallow it */
	BroadcastPacket(
	  M_MAP, 3, FW_W(Fw),FW_W_FRAME(Fw), (unsigned long)Fw);
#endif
      }
      MyXUngrabServer(dpy);
      break;

    case IconicState:
      if (isIconifiedByParent ||
	  ((tmp = get_transientfor_fvwmwindow(Fw)) && IS_ICONIFIED(tmp)))
      {
	isIconifiedByParent = False;
	SET_ICONIFIED_BY_PARENT(Fw, 1);
      }
      if (USE_ICON_POSITION_HINT(Fw) && Fw->wmhints &&
	  (Fw->wmhints->flags & IconPositionHint))
      {
	Iconify(Fw, Fw->wmhints->icon_x,
		Fw->wmhints->icon_y);
      }
      else
      {
	Iconify(Fw, 0, 0);
      }
      if (is_new_window)
      {
	/* the window will not be mapped - fake an UnmapNotify event */
	fake_map_unmap_notify(Fw, UnmapNotify);
      }
      break;
    }
  }
  if (IS_SHADED(Fw))
  {
    BroadcastPacket(M_WINDOWSHADE, 3, FW_W(Fw), FW_W_FRAME(Fw),
                    (unsigned long)Fw);
  }

  if (!IS_ICONIFIED(Fw) && (sf = get_focus_window()) &&
      sf != Fw && !is_on_top_of_layer(sf))
  {
    if (Fw->Desk == Scr.CurrentDesk &&
	Fw->frame_g.x + Fw->frame_g.width > sf->frame_g.x &&
	sf->frame_g.x + sf->frame_g.width > Fw->frame_g.x &&
	Fw->frame_g.y + Fw->frame_g.height > sf->frame_g.y &&
	sf->frame_g.y + sf->frame_g.height > Fw->frame_g.y)
    {
      /* The newly mapped window overlaps the focused window. Make sure
       * ClickToFocusRaises and MouseFocusClickRaises work again.
       *
       * Note: There are many conditions under which we do not have to call
       * focus_grab_buttons(), but it is not worth the effort to write them
       * down here.  Rather do some unnecessary work in this function. */
      focus_grab_buttons(sf, True);
    }
  }
  if (is_menu)
  {
    SET_MAPPED(Fw, 1);
    SET_MAP_PENDING(Fw, 0);
  }

  /* Just to be on the safe side, we make sure that STARTICONIC
     can only influence the initial transition from withdrawn state. */
  SET_DO_START_ICONIC(Fw, 0);
  if (DO_DELETE_ICON_MOVED(Fw))
  {
    SET_DELETE_ICON_MOVED(Fw, 0);
    SET_ICON_MOVED(Fw, 0);
  }
  /* Clean out the global so that it isn't used on additional map events. */
  isIconicState = DontCareState;
  EWMH_SetClientList();
  GNOME_SetClientList();
#if 0
{void setup_window_name(FvwmWindow *tmp_win);
usleep(200000);
setup_window_name(Fw);}
fprintf(stderr,"-----\n");
#endif
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleMapNotify - MapNotify event handler
 *
 ***********************************************************************/
void HandleMapNotify(void)
{
  Bool is_on_this_page = False;

  DBUG("HandleMapNotify","Routine Entered");

  if (!Fw)
  {
    if((Event.xmap.override_redirect == True)&&
       (Event.xmap.window != Scr.NoFocusWin))
    {
      XSelectInput(dpy, Event.xmap.window, XEVMASK_ORW);
      Scr.UnknownWinFocused = Event.xmap.window;
    }
    return;
  }

  /* Except for identifying over-ride redirect window mappings, we
   * don't need or want windows associated with the substructurenotifymask */
  if(Event.xmap.event != Event.xmap.window)
    return;

  SET_MAP_PENDING(Fw, 0);
  /* don't map if the event was caused by a de-iconify */
  if (IS_DEICONIFY_PENDING(Fw))
  {
    return;
  }

  /*
      Make sure at least part of window is on this page
      before giving it focus...
  */
  is_on_this_page = IsRectangleOnThisPage(&(Fw->frame_g), Fw->Desk);

  /*
   * Need to do the grab to avoid race condition of having server send
   * MapNotify to client before the frame gets mapped; this is bad because
   * the client would think that the window has a chance of being viewable
   * when it really isn't.
   */
  MyXGrabServer (dpy);
  if (FW_W_ICON_TITLE(Fw))
    XUnmapWindow(dpy, FW_W_ICON_TITLE(Fw));
  if (FW_W_ICON_PIXMAP(Fw) != None)
    XUnmapWindow(dpy, FW_W_ICON_PIXMAP(Fw));
  XMapSubwindows(dpy, FW_W_FRAME(Fw));
  XMapSubwindows(dpy, Fw->decor_w);
  if (Fw->Desk == Scr.CurrentDesk)
  {
    XMapWindow(dpy, FW_W_FRAME(Fw));
  }
  if(IS_ICONIFIED(Fw))
    BroadcastPacket(M_DEICONIFY, 3,
                    FW_W(Fw), FW_W_FRAME(Fw), (unsigned long)Fw);
  else
    BroadcastPacket(M_MAP, 3,
                    FW_W(Fw),FW_W_FRAME(Fw), (unsigned long)Fw);

  if((!IS_TRANSIENT(Fw) && DO_GRAB_FOCUS(Fw)) ||
     (IS_TRANSIENT(Fw) && DO_GRAB_FOCUS_TRANSIENT(Fw) &&
      get_focus_window() &&
      FW_W(get_focus_window()) == FW_W_TRANSIENTFOR(Fw)))
  {
    if (is_on_this_page)
    {
      SetFocusWindow(Fw, True, True);
    }
  }
  if (!HAS_BORDER(Fw) && !HAS_TITLE(Fw) &&
      is_window_border_minimal(Fw))
  {
    DrawDecorations(
      Fw, DRAW_ALL, False, True, Fw->decor_w, CLEAR_ALL);
  }
  else if (Fw == get_focus_window() && Fw != Scr.Hilite)
  {
    /* BUG 679: must redraw decorations here to make sure the window is properly
     * hilighted after being de-iconified by a key press. */
    DrawDecorations(
      Fw, DRAW_ALL, True, True, None, CLEAR_ALL);
  }
  MyXUngrabServer (dpy);
  SET_MAPPED(Fw, 1);
  SET_ICONIFIED(Fw, 0);
  SET_ICON_UNMAPPED(Fw, 0);
  if (DO_ICONIFY_AFTER_MAP(Fw))
  {
    /* finally, if iconification was requested before the window was mapped,
     * request it now. */
    Iconify(Fw, 0, 0);
    SET_ICONIFY_AFTER_MAP(Fw, 0);
  }
}


Bool check_map_request(Display *display, XEvent *event, char *arg)
{
  unsigned long *rc;

  if (event->type != MapRequest)
  {
    return False;
  }
  if (event->xmaprequest.window != *(unsigned long *)arg)
  {
    return False;
  }
  rc = (unsigned long *)arg;
  (*arg)++;

  /* Yes, it is correct that this function always returns False.  See
   * comment below. */
  return False;
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleUnmapNotify - UnmapNotify event handler
 *
 ************************************************************************/
void HandleUnmapNotify(void)
{
  int dstx, dsty;
  Window dumwin;
  XEvent dummy;
  int    weMustUnmap;
  int    focus_grabbed = 0;
  Bool must_return = False;

  DBUG("HandleUnmapNotify","Routine Entered");

  /*
   * Don't ignore events as described below.
   */
  if(Event.xunmap.event != Event.xunmap.window &&
     (Event.xunmap.event != Scr.Root || !Event.xunmap.send_event))
  {
    must_return = True;
  }

  /*
   * The July 27, 1988 ICCCM spec states that a client wishing to switch
   * to WithdrawnState should send a synthetic UnmapNotify with the
   * event field set to (pseudo-)root, in case the window is already
   * unmapped (which is the case for fvwm for IconicState).  Unfortunately,
   * we looked for the FvwmContext using that field, so try the window
   * field also.
   */
  weMustUnmap = 0;
  if (!Fw)
  {
    Event.xany.window = Event.xunmap.window;
    weMustUnmap = 1;
    if (XFindContext(dpy, Event.xany.window,
		     FvwmContext, (caddr_t *)&Fw) == XCNOENT)
    {
      Fw = NULL;
      return;
    }
  }
  if (Event.xunmap.window == FW_W_FRAME(Fw))
  {
    SET_DEICONIFY_PENDING(Fw , 0);
  }
  if (must_return)
    return;

  if (weMustUnmap)
  {
    unsigned long win = (unsigned long)Event.xunmap.window;
    Bool is_map_request_pending;

    /* Using XCheckTypedWindowEvent() does not work here.  I don't have the
     * slightest idea why, but using XCheckIfEvent() with the appropriate
     * predicate procedure works fine. */
    XCheckIfEvent(dpy, &dummy, check_map_request, (char *)&win);
    /* Unfortunately, there is no procedure in X that simply tests if an
     * event of a certain type in on the queue without waiting and without
     * removing it from the queue.  XCheck...Event() does not wait but
     * removes the event while XPeek...() does not remove the event but
     * waits.  Now, this is a *real* hack:
     *
     * The predicate procedure used in the call always returns False.  Thus,
     * no event is ever removed from the queue.  But when it is called with
     * the correct event type and window, it adds one to the unsigned long
     * pointer that was passed in as its last argument.  Of course this is
     * using totally undocumented 'features' and may be very inefficient
     * (im many events are pending). */
    is_map_request_pending = (win != (unsigned long)Event.xunmap.window);
    if (!is_map_request_pending)
    {
      XUnmapWindow(dpy, Event.xunmap.window);
    }
  }
  if(Fw ==  Scr.Hilite)
  {
    Scr.Hilite = NULL;
  }
  focus_grabbed = (Fw == get_focus_window()) &&
    ((!IS_TRANSIENT(Fw) && DO_GRAB_FOCUS(Fw)) ||
     (IS_TRANSIENT(Fw) && DO_GRAB_FOCUS_TRANSIENT(Fw)));
  restore_focus_after_unmap(Fw, False);
  if (!IS_MAPPED(Fw) && !IS_ICONIFIED(Fw))
  {
    return;
  }

  /*
   * The program may have unmapped the client window, from either
   * NormalState or IconicState.  Handle the transition to WithdrawnState.
   *
   * We need to reparent the window back to the root (so that fvwm exiting
   * won't cause it to get mapped) and then throw away all state (pretend
   * that we've received a DestroyNotify).
   */
  if (!XCheckTypedWindowEvent (dpy, Event.xunmap.window, DestroyNotify, &dummy)
   && XTranslateCoordinates (dpy, Event.xunmap.window, Scr.Root,
			     0, 0, &dstx, &dsty, &dumwin))
  {
    XEvent ev;

    MyXGrabServer(dpy);
    SetMapStateProp (Fw, WithdrawnState);
    EWMH_RestoreInitialStates(Fw, Event.type);
    if (XCheckTypedWindowEvent(dpy, Event.xunmap.window, ReparentNotify, &ev))
    {
      if (Fw->old_bw)
	XSetWindowBorderWidth (dpy, Event.xunmap.window, Fw->old_bw);
      if((!IS_ICON_SUPPRESSED(Fw))&&
	 (Fw->wmhints && (Fw->wmhints->flags & IconWindowHint)))
	XUnmapWindow (dpy, Fw->wmhints->icon_window);
    }
    else
    {
      RestoreWithdrawnLocation (Fw, False, Scr.Root);
    }
    if (!IS_TEAR_OFF_MENU(Fw))
    {
      XRemoveFromSaveSet (dpy, Event.xunmap.window);
      XSelectInput (dpy, Event.xunmap.window, NoEventMask);
    }
    XSync(dpy, 0);
    MyXUngrabServer(dpy);
  }
  destroy_window(Fw);		/* do not need to mash event before */
  if (focus_grabbed)
  {
    CoerceEnterNotifyOnCurrentWindow();
  }
  EWMH_ManageKdeSysTray(Event.xunmap.window, Event.type);
  EWMH_WindowDestroyed();
  GNOME_SetClientList();
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleReparentNotify - ReparentNotify event handler
 *
 ************************************************************************/
void HandleReparentNotify(void)
{
  if (!Fw)
    return;
  if (Event.xreparent.parent == Scr.Root)
  {
    /* Ignore reparenting to the root window.  In some cases these events are
     * selected although the window is no longer managed. */
    return;
  }
  if (Event.xreparent.parent != FW_W_FRAME(Fw))
  {
    /* window was reparented by someone else, destroy the frame */
    SetMapStateProp(Fw, WithdrawnState);
    EWMH_RestoreInitialStates(Fw, Event.type);
    if (!IS_TEAR_OFF_MENU(Fw))
    {
      XRemoveFromSaveSet(dpy, Event.xreparent.window);
      XSelectInput (dpy, Event.xreparent.window, NoEventMask);
    }
    else
    {
      XSelectInput (dpy, Event.xreparent.window, XEVMASK_MENUW);
    }
    discard_events(XEVMASK_FRAMEW);
    destroy_window(Fw);
    EWMH_ManageKdeSysTray(Event.xreparent.window, Event.type);
    EWMH_WindowDestroyed();
  }

  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleButtonPress - ButtonPress event handler
 *
 ***********************************************************************/
void HandleButtonPress(void)
{
  int LocalContext;
  char *action;
  Window OldPressedW;
  Window eventw;
  Bool do_regrab_buttons = False;
  Bool do_pass_click;
  Bool has_binding = False;

  DBUG("HandleButtonPress","Routine Entered");

  GrabEm(CRS_NONE, GRAB_PASSIVE);
  if (!Fw &&
      Event.xbutton.window != Scr.PanFrameTop.win &&
      Event.xbutton.window != Scr.PanFrameBottom.win &&
      Event.xbutton.window != Scr.PanFrameLeft.win &&
      Event.xbutton.window != Scr.PanFrameRight.win &&
      Event.xbutton.window != Scr.Root)
  {
    /* event in unmanaged window or subwindow of a client */
    XSync(dpy,0);
    XAllowEvents(dpy,ReplayPointer,CurrentTime);
    XSync(dpy,0);
    UngrabEm(GRAB_PASSIVE);
    return;
  }
  if (Event.xbutton.subwindow != None &&
      (Fw == None || Event.xany.window != FW_W(Fw)))
  {
    eventw = Event.xbutton.subwindow;
  }
  else
  {
    eventw = Event.xany.window;
  }
  if (!XGetGeometry(dpy, eventw, &JunkRoot, &JunkX, &JunkY,
		    &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth))
  {
    /* The window has already died. Just pass the event to the application. */
    XSync(dpy,0);
    XAllowEvents(dpy,ReplayPointer,CurrentTime);
    XSync(dpy,0);
    UngrabEm(GRAB_PASSIVE);
    return;
  }
  if (Fw && HAS_NEVER_FOCUS(Fw))
  {
    /* It might seem odd to try to focus a window that never is given focus by
     * fvwm, but the window might want to take focus itself, and SetFocus will
     * tell it to do so in this case instead of giving it focus. */
    SetFocusWindow(Fw, True, True);
  }
  /* click to focus stuff goes here */
  if((Fw)&&(HAS_CLICK_FOCUS(Fw))&&(Fw != Scr.Ungrabbed))
  {
    if (Fw != get_focus_window())
    {
      SetFocusWindow(Fw, True, True);
    }
    /* RBW - 12/09/.1999- I'm not sure we need to check both cases, but
       I'll leave this as is for now.  */
    if (!DO_NOT_RAISE_CLICK_FOCUS_CLICK(Fw)
#if 0
	/* DV - this forces that every focus click on the decorations raises
	 * the window.  This somewhat negates the ClickToFocusRaisesOff style.
	 */
	||
	((Event.xany.window != FW_W(Fw))&&
	 (Event.xbutton.subwindow != FW_W(Fw))&&
	 (Event.xany.window != FW_W_PARENT(Fw))&&
	 (Event.xbutton.subwindow != FW_W_PARENT(Fw)))
#endif
      )
    {
      /* We can't raise the window immediately because the action bound to the
       * click might be "Lower" or "RaiseLower". So mark the window as
       * scheduled to be raised after the binding is executed. Functions that
       * modify the stacking order will reset this flag. */
      SET_SCHEDULED_FOR_RAISE(Fw, 1);
      do_regrab_buttons = True;
    }

    Context = GetContext(Fw, &Event, &PressedW);
    if (!IS_ICONIFIED(Fw) && Context == C_WINDOW)
    {
      if (Fw && IS_SCHEDULED_FOR_RAISE(Fw))
      {
	RaiseWindow(Fw);
	SET_SCHEDULED_FOR_RAISE(Fw, 0);
      }
      if (do_regrab_buttons)
	focus_grab_buttons(Fw, (Fw == get_focus_window()));
      XSync(dpy,0);
      /* Pass click event to just clicked to focus window? Do not swallow the
       * click if the window didn't accept the focus. */
      if (!DO_NOT_PASS_CLICK_FOCUS_CLICK(Fw) ||
	  get_focus_window() != Fw)
      {
	XAllowEvents(dpy,ReplayPointer,CurrentTime);
	/*	return;*/
      }
      else /* don't pass click to just focused window */
      {
	XAllowEvents(dpy,AsyncPointer,CurrentTime);
      }
      UngrabEm(GRAB_PASSIVE);
    }
    if (!IS_ICONIFIED(Fw))
    {
      DrawDecorations(Fw, DRAW_ALL, True, True, PressedW, CLEAR_ALL);
    }
  }
  else if (Fw && Event.xbutton.window == FW_W_PARENT(Fw) &&
	   (HAS_SLOPPY_FOCUS(Fw) || HAS_MOUSE_FOCUS(Fw) ||
	    HAS_NEVER_FOCUS(Fw)) &&
	   DO_RAISE_MOUSE_FOCUS_CLICK(Fw))
  {
    FvwmWindow *tmp = Scr.Ungrabbed;

    /*
      RBW - Release the Parent grab here (whether we raise or not). We
      have to wait till this point or we would miss the raise click, which
      is not contemporaneous with the focus change.
      Scr.Ungrabbed should always be NULL here. I don't know anything
      useful we could do if it's not, other than ignore this window.
    */
    if (!is_on_top_of_layer(Fw) &&
        MaskUsedModifiers(Event.xbutton.state) == 0)
    {
      if (!DO_IGNORE_MOUSE_FOCUS_CLICK_MOTION(Fw))
      {
	/* raise immediately and pass the click to the application */
	RaiseWindow(Fw);
	focus_grab_buttons(Fw, True);
	Scr.Ungrabbed = tmp;
	XSync(dpy,0);
	XAllowEvents(dpy,ReplayPointer,CurrentTime);
	XSync(dpy,0);
	UngrabEm(GRAB_PASSIVE);
	return;
      }
      else
      {
	Bool is_click = False;
	Bool is_done = False;
	int x0 = Event.xbutton.x_root;
	int y0 = Event.xbutton.y_root;
	int x = Event.xbutton.x_root;
	int y = Event.xbutton.y_root;
	unsigned int mask;

	/* pass further events to the application and check if a button release
	 * or motion event occurs next */
	XAllowEvents(dpy, ReplayPointer, CurrentTime);
	/* query the pointer to do this. we can't check for events here since
	 * the events are still needed if the pointer moves */
	while (!is_done && XQueryPointer(
		       dpy, Scr.Root, &JunkRoot, &JunkChild,
		       &JunkX, &JunkY, &x, &y, &mask) == True)
	{
	  if ((mask & DEFAULT_ALL_BUTTONS_MASK) == 0)
	  {
	    /* all buttons are released */
	    is_done = True;
	    is_click = True;
	  }
	  else if (abs(x - x0) >= Scr.MoveThreshold ||
		   abs(y - y0) >= Scr.MoveThreshold)
	  {
	    /* the pointer has moved */
	    is_done = True;
	    is_click = False;
	  }
	  else
	  {
	    usleep(20000);
	  }
	}
	if (is_done == False || is_click == True)
	{
	  /* raise the window and exit */
	  RaiseWindow(Fw);
	  focus_grab_buttons(Fw, True);
	  Scr.Ungrabbed = tmp;
	  XSync(dpy,0);
	  UngrabEm(GRAB_PASSIVE);
	  return;
	}
	else
	{
	  /* the pointer was moved, process event normally */
	}
      }
    }
    focus_grab_buttons(Fw, True);
    Scr.Ungrabbed = tmp;
  }

  Context = GetContext(Fw, &Event, &PressedW);
  LocalContext = Context;
  STROKE_CODE(stroke_init());
  STROKE_CODE(send_motion = TRUE);
  /* need to search for an appropriate mouse binding */
  action = CheckBinding(Scr.AllBindings, STROKE_ARG(0) Event.xbutton.button,
			Event.xbutton.state, GetUnusedModifiers(), Context,
			MOUSE_BINDING);
  if (action && *action)
  {
    has_binding = True;
  }

  if (Fw && has_binding)
  {
    if (Context == C_TITLE)
    {
      DrawDecorations(
        Fw, DRAW_TITLE, (Scr.Hilite == Fw), True, PressedW,
	CLEAR_ALL);
    }
    else if (Context & (C_LALL | C_RALL))
    {
      DrawDecorations(
        Fw, DRAW_BUTTONS, (Scr.Hilite == Fw),
	True, PressedW, CLEAR_ALL);
    }
    else
    {
      DrawDecorations(
        Fw, DRAW_FRAME, (Scr.Hilite == Fw),
        (HAS_DEPRESSABLE_BORDER(Fw) && PressedW != None),
	PressedW, CLEAR_ALL);
    }
  }

  /* we have to execute a function or pop up a menu */
  ButtonWindow = Fw;
  do_pass_click = True;
  if (action && *action)
  {
    if (Fw && IS_ICONIFIED(Fw))
    {
      /* release the pointer since it can't do harm over an icon */
      XAllowEvents(dpy, AsyncPointer, CurrentTime);
    }
    old_execute_function(NULL, action, Fw, &Event, Context, -1, 0, NULL);
    if (Context != C_WINDOW && Context != C_NO_CONTEXT)
    {
      WaitForButtonsUp(True);
      do_pass_click = False;
    }
  }
  else if (Scr.Root == Event.xany.window)
  {
    /*
     * do gnome buttonpress forwarding if win == root
     */
    GNOME_ProxyButtonEvent(&Event);
    do_pass_click = False;
  }

  if (do_pass_click)
  {
    XSync(dpy,0);
    XAllowEvents(dpy,ReplayPointer,CurrentTime);
    XSync(dpy,0);
  }

  if (ButtonWindow && IS_SCHEDULED_FOR_RAISE(ButtonWindow) && has_binding)
  {
    /* now that we know the action did not restack the window we can raise it.
     */
    RaiseWindow(ButtonWindow);
    SET_SCHEDULED_FOR_RAISE(ButtonWindow, 0);
  }
  if (do_regrab_buttons)
  {
    focus_grab_buttons(Fw, (Fw == get_focus_window()));
  }

  OldPressedW = PressedW;
  PressedW = None;
  if (ButtonWindow && check_if_fvwm_window_exists(ButtonWindow) && has_binding)
  {
    if (LocalContext == C_TITLE)
      DrawDecorations(
        ButtonWindow, DRAW_TITLE, (Scr.Hilite == ButtonWindow),
	True, None, CLEAR_ALL);
    else if (LocalContext & (C_LALL | C_RALL))
      DrawDecorations(
        ButtonWindow, DRAW_BUTTONS, (Scr.Hilite == ButtonWindow), True,
        OldPressedW, CLEAR_ALL);
    else
      DrawDecorations(
        ButtonWindow, DRAW_FRAME, (Scr.Hilite == ButtonWindow),
        HAS_DEPRESSABLE_BORDER(ButtonWindow), None, CLEAR_ALL);
  }
  ButtonWindow = NULL;
  UngrabEm(GRAB_PASSIVE);

  return;
}

#ifdef HAVE_STROKE
/***********************************************************************
 *
 *  Procedure:
 *	HandleButtonRelease - ButtonRelease event handler
 *
 ************************************************************************/
void HandleButtonRelease()
{
   char *action;
   int real_modifier;
   Window dummy;


   DBUG("HandleButtonRelease","Routine Entered");

   send_motion = FALSE;
   stroke_trans (sequence);

   DBUG("HandleButtonRelease",sequence);

   Context = GetContext(Fw,&Event, &dummy);

   /*  Allows modifier to work (Only R context works here). */
   real_modifier = Event.xbutton.state - (1 << (7 + Event.xbutton.button));

   /* need to search for an appropriate stroke binding */
   action = CheckBinding(
     Scr.AllBindings, sequence, Event.xbutton.button, real_modifier,
     GetUnusedModifiers(), Context, STROKE_BINDING);
   /* got a match, now process it */
   if (action != NULL && (action[0] != 0))
   {
     old_execute_function(NULL, action, Fw, &Event, Context, -1, 0, NULL);
     WaitForButtonsUp(True);
   }
   else
   {
     /*
      * do gnome buttonpress forwarding if win == root
      */
      if (Scr.Root == Event.xany.window)
      {
          GNOME_ProxyButtonEvent(&Event);
      }
   }

}

/***********************************************************************
 *
 *  Procedure:
 *	HandleMotionNotify - MotionNotify event handler
 *
 ************************************************************************/
void HandleMotionNotify()
{
  DBUG("HandleMotionNotify","Routine Entered");

  if (send_motion == TRUE)
    stroke_record (Event.xmotion.x,Event.xmotion.y);
}

#endif /* HAVE_STROKE */

/***********************************************************************
 *
 *  Procedure:
 *	HandleEnterNotify - EnterNotify event handler
 *
 ************************************************************************/
void HandleEnterNotify(void)
{
  XEnterWindowEvent *ewp = &Event.xcrossing;
  XEvent d;
  FvwmWindow *sf;
  static Bool is_initial_ungrab_pending = True;
  Bool is_tear_off_menu;

  DBUG("HandleEnterNotify","Routine Entered");

  if (Scr.flags.is_wire_frame_displayed)
  {
    /* Ignore EnterNotify events while a window is resized or moved as a wire
     * frame; otherwise the window list may be screwed up. */
    return;
  }

  if (Fw)
  {
    if (ewp->window != FW_W_FRAME(Fw) && ewp->window != FW_W_PARENT(Fw) &&
        ewp->window != FW_W(Fw) && ewp->window != Fw->decor_w &&
	ewp->window != FW_W_ICON_TITLE(Fw) &&
	ewp->window != FW_W_ICON_PIXMAP(Fw))
    {
      /* Ignore EnterNotify that received by any of the sub windows
       * that don't handle this event.  unclutter triggers these
       * events sometimes, re focusing an unfocused window under
       * the pointer */
      return;
    }
  }
  if (ewp->mode == NotifyGrab)
  {
    return;
  }
  else if (ewp->mode == NotifyNormal)
  {
    if (ewp->detail == NotifyNonlinearVirtual && ewp->focus == False &&
	ewp->subwindow != None)
    {
      /* This takes care of some buggy apps that forget that one of their
       * dialog subwindows has the focus after popping up a selection list
       * several times (ddd, netscape). I'm not convinced that this does not
       * break something else. */
      refresh_focus(Fw);
    }
  }
  else if (ewp->mode == NotifyUngrab)
  {
    /* Ignore events generated by grabbing or ungrabbing the pointer.  However,
     * there is no way to prevent the client application from handling this
     * event and, for example, grabbing the focus.  This will interfere with
     * functions that transferred the focus to a different window. */
    if (is_initial_ungrab_pending)
    {
      is_initial_ungrab_pending = False;
      xcrossing_last_grab_window = NULL;
    }
    else
    {
      if (ewp->detail == NotifyNonlinearVirtual && ewp->focus == False &&
	  ewp->subwindow != None)
      {
	/* see comment above */
	refresh_focus(Fw);
      }
      if (Fw && Fw == xcrossing_last_grab_window)
      {
	return;
      }
    }
  }
  if (Fw)
  {
    is_initial_ungrab_pending = False;
  }

  /* look for a matching leaveNotify which would nullify this enterNotify */
  /*
   * RBW - if we're in startup, this is a coerced focus, so we don't
   * want to save the event time, or exit prematurely.
   *
   * Ignore LeaveNotify events for tear out menus - handled by menu code
   */
  is_tear_off_menu =
    (Fw && IS_TEAR_OFF_MENU(Fw) && ewp->window == FW_W(Fw));
  if (!fFvwmInStartup && !is_tear_off_menu &&
      XCheckTypedWindowEvent(dpy, ewp->window, LeaveNotify, &d))
  {
    StashEventTime(&d);
    if((d.xcrossing.mode==NotifyNormal)&&
       (d.xcrossing.detail!=NotifyInferior))
    {
      return;
    }
  }

  if (ewp->window == Scr.Root)
  {
    FvwmWindow *lf = get_last_screen_focus_window();

    if (!Scr.flags.is_pointer_on_this_screen)
    {
      Scr.flags.is_pointer_on_this_screen = 1;
      if (lf && lf != &Scr.FvwmRoot &&
	  (HAS_SLOPPY_FOCUS(lf) || HAS_CLICK_FOCUS(lf)))
      {
	SetFocusWindow(lf, True, True);
      }
      else if (lf != &Scr.FvwmRoot)
      {
	ForceDeleteFocus(1);
      }
      else
      {
	/* This was the first EnterNotify event for the root window - ignore */
      }
      set_last_screen_focus_window(NULL);
    }
    else if (!(sf = get_focus_window()) || HAS_MOUSE_FOCUS(sf))
    {
      DeleteFocus(True, True);
    }
    if (Scr.ColormapFocus == COLORMAP_FOLLOWS_MOUSE)
    {
      InstallWindowColormaps(NULL);
    }
    return;
  }
  else
  {
    Scr.flags.is_pointer_on_this_screen = 1;
  }

  /* an EnterEvent in one of the PanFrameWindows activates the Paging */
  if (ewp->window==Scr.PanFrameTop.win
      || ewp->window==Scr.PanFrameLeft.win
      || ewp->window==Scr.PanFrameRight.win
      || ewp->window==Scr.PanFrameBottom.win )
  {
    int delta_x=0, delta_y=0;

    /* this was in the HandleMotionNotify before, HEDU */
    Scr.flags.is_pointer_on_this_screen = 1;
    HandlePaging(Scr.EdgeScrollX,Scr.EdgeScrollY,
                 &ewp->x_root,&ewp->y_root,
                 &delta_x,&delta_y,True,True,False);
    return;
  }
  /* make sure its for one of our windows */
  if (!Fw)
  {
    /* handle a subwindow cmap */
    EnterSubWindowColormap(Event.xany.window);
    return;
  }

  BroadcastPacket(
    MX_ENTER_WINDOW, 3, FW_W(Fw), FW_W_FRAME(Fw), (unsigned long)Fw);
  sf = get_focus_window();
  if (sf && Fw != sf && HAS_MOUSE_FOCUS(sf))
  {
    DeleteFocus(True, True);
  }
  if (HAS_MOUSE_FOCUS(Fw) || HAS_SLOPPY_FOCUS(Fw))
  {
    SetFocusWindow(Fw, True, True);
  }
  else if (HAS_NEVER_FOCUS(Fw))
  {
    /* Give the window a chance to grab the buttons needed for raise-on-click */
    if (sf != Fw)
    {
      focus_grab_buttons(Fw, False);
      focus_grab_buttons(sf, True);
    }
  }
  else if (HAS_CLICK_FOCUS(Fw) && Fw == get_focus_window() &&
	   do_accept_input_focus(Fw))
  {
    /* We have to refresh the focus window here in case we left the focused
     * fvwm window.  Motif apps may lose the input focus otherwise.  But do not
     * try to refresh the focus of applications that want to handle it
     * themselves. */
    FOCUS_SET(FW_W(Fw));
  }
  if (Scr.ColormapFocus == COLORMAP_FOLLOWS_MOUSE)
  {
    if((!IS_ICONIFIED(Fw))&&(Event.xany.window == FW_W(Fw)))
    {
      InstallWindowColormaps(Fw);
    }
    else
    {
      InstallWindowColormaps(NULL);
    }
  }
  /* We get an EnterNotify with mode == UnGrab when fvwm releases
     the grab held during iconification. We have to ignore this,
     or icon title will be initially raised. */
  if (IS_ICONIFIED(Fw) && (ewp->mode == NotifyNormal))
  {
    SET_ICON_ENTERED(Fw,1);
    DrawIconWindow(Fw);
  }
  /* Check for tear off menus */
  if (is_tear_off_menu)
  {
    menu_enter_tear_off_menu(Fw);
  }

  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleLeaveNotify - LeaveNotify event handler
 *
 ************************************************************************/
void HandleLeaveNotify(void)
{
  DBUG("HandleLeaveNotify","Routine Entered");

  /* Ignore LeaveNotify events while a window is resized or moved as a wire
   * frame; otherwise the window list may be screwed up. */
  if (Scr.flags.is_wire_frame_displayed)
  {
    return;
  }
  if (Event.xcrossing.mode != NotifyNormal)
  {
    /* Ignore events generated by grabbing or ungrabbing the pointer.  However,
     * there is no way to prevent the ckient application from handling this
     * event and, for example, grabbing the focus.  This will interfere with
     * functions that transferred the focus to a different window. */
    if (Event.xcrossing.mode == NotifyGrab && Fw &&
	(Event.xcrossing.window == Fw->decor_w ||
	 Event.xcrossing.window == FW_W(Fw) ||
	 Event.xcrossing.window == FW_W_ICON_TITLE(Fw) ||
	 Event.xcrossing.window == FW_W_ICON_PIXMAP(Fw)))
    {
      xcrossing_last_grab_window = Fw;
    }
#ifdef FOCUS_EXPANDS_TITLE
    if (Fw && IS_ICONIFIED(Fw))
    {
      SET_ICON_ENTERED(Fw, 0);
      DrawIconWindow(Fw);
    }
#endif
    return;
  }
  /* CDE-like behaviour of raising the icon title if the icon
     gets the focus (in particular if the cursor is over the icon) */
  if (Fw && IS_ICONIFIED(Fw))
  {
    SET_ICON_ENTERED(Fw,0);
    DrawIconWindow (Fw);
  }

  /* If we leave the root window, then we're really moving
   * another screen on a multiple screen display, and we
   * need to de-focus and unhighlight to make sure that we
   * don't end up with more than one highlighted window at a time */
  if(Event.xcrossing.window == Scr.Root
     /* domivogt (16-May-2000): added this test because somehow fvwm sometimes
      * gets a LeaveNotify on the root window although it is single screen. */
     && Scr.NumberOfScreens > 1)
  {
    if(Event.xcrossing.mode == NotifyNormal)
    {
      if (Event.xcrossing.detail != NotifyInferior)
      {
	FvwmWindow *sf = get_focus_window();

	Scr.flags.is_pointer_on_this_screen = 0;
	set_last_screen_focus_window(sf);
	if (sf != NULL)
	{
	  DeleteFocus(True, True);
	}
	if (Scr.Hilite != NULL)
	  DrawDecorations(Scr.Hilite, DRAW_ALL, False, True, None, CLEAR_ALL);
      }
    }
  }
  else
  {
    /* handle a subwindow cmap */
    LeaveSubWindowColormap(Event.xany.window);
  }
  if (Fw != NULL)
  {
    BroadcastPacket(
      MX_LEAVE_WINDOW, 3, FW_W(Fw), FW_W_FRAME(Fw), (unsigned long)Fw);
  }

  return;
}



/***********************************************************************
 *
 *  Procedure:
 *	HandleConfigureRequest - ConfigureRequest event handler
 *
 ************************************************************************/
void HandleConfigureRequest(void)
{
  rectangle new_g;
  int dx;
  int dy;
  int dw;
  int dh;
  int constr_w;
  int constr_h;
  int oldnew_w;
  int oldnew_h;
  XWindowChanges xwc;
  unsigned long xwcm;
  XConfigureRequestEvent *cre = &Event.xconfigurerequest;
  Bool do_send_event = False;

  DBUG("HandleConfigureRequest","Routine Entered");

  /*
   * Event.xany.window is Event.xconfigurerequest.parent, so Fw will
   * be wrong
   */
  Event.xany.window = cre->window;	/* mash parent field */
  if (XFindContext (dpy, cre->window, FvwmContext, (caddr_t *) &Fw) ==
      XCNOENT)
    Fw = NULL;

#define NO_EXPERIMENTAL_ANTI_RACE_CONDITION_CODE
  /* This is not a good idea because this interferes with changes in the size
   * hints of the window.  However, it is impossible to be completely safe here.
   * For example, if the client changes the size inc, then resizes its of window
   * and then changes the size inc again - all in one batch - then the WM will
   * read the *second* size inc upon the *first* event and use the wrong one in
   * the ConfigureRequest calculations. */
#ifdef EXPERIMENTAL_ANTI_RACE_CONDITION_CODE
  /* merge all pending ConfigureRequests for the window into a single event */
  if (Fw)
  {
    XEvent e;
    XConfigureRequestEvent *ecre;

    /* free some CPU */
    usleep(1);
    while (XCheckTypedWindowEvent(dpy, cre->window, ConfigureRequest, &e))
    {
      unsigned long vma;
      unsigned long vmo;
      const unsigned long xm = CWX | CWWidth;
      const unsigned long ym = CWY | CWHeight;

      ecre = &e.xconfigurerequest;
      vma = cre->value_mask & ecre->value_mask;
      vmo = cre->value_mask | ecre->value_mask;
      if (((vma & xm) == 0 && (vmo & xm) == xm) ||
	  ((vma & ym) == 0 && (vmo & ym) == ym))
      {
	/* can't merge events since location of window might get screwed up */
	XPutBackEvent(dpy, &e);
	break;
      }
      if (ecre->value_mask & CWX)
      {
	cre->value_mask |= CWX;
	cre->x = ecre->x;
      }
      if (ecre->value_mask & CWY)
      {
	cre->value_mask |= CWY;
	cre->y = ecre->y;
      }
      if (ecre->value_mask & CWWidth)
      {
	cre->value_mask |= CWWidth;
	cre->width = ecre->width;
      }
      if (ecre->value_mask & CWHeight)
      {
	cre->value_mask |= CWHeight;
	cre->height = ecre->height;
      }
      if (ecre->value_mask & CWBorderWidth)
      {
	cre->value_mask |= CWBorderWidth;
	cre->border_width = ecre->border_width;
      }
      if (ecre->value_mask & CWStackMode)
      {
	cre->value_mask &= !CWStackMode;
	cre->value_mask |= (ecre->value_mask & CWStackMode);
	cre->above = ecre->above;
	cre->detail = ecre->detail;
	/* srt (28-Apr-2001): Tk needs a ConfigureNotify event after a raise,
	 * otherwise it would hang for two seconds */
	new_g = Fw->frame_g;
	do_send_event = True;
      }
    } /* while */
  } /* if */
#endif

  /*
   * According to the July 27, 1988 ICCCM draft, we should ignore size and
   * position fields in the WM_NORMAL_HINTS property when we map a window.
   * Instead, we'll read the current geometry.  Therefore, we should respond
   * to configuration requests for windows which have never been mapped.
   */
  if (!Fw || cre->window == FW_W_ICON_TITLE(Fw) ||
      cre->window == FW_W_ICON_PIXMAP(Fw))
  {
    xwcm = cre->value_mask &
      (CWX | CWY | CWWidth | CWHeight | CWBorderWidth);
    xwc.x = cre->x;
    xwc.y = cre->y;
    if (Fw && FW_W_ICON_PIXMAP(Fw) == cre->window)
    {
      set_icon_picture_size(
	Fw, cre->width + 2 * cre->border_width,
	cre->height + 2 * cre->border_width);
    }
    if (Fw)
    {
      set_icon_position(Fw, cre->x, cre->y);
      broadcast_icon_geometry(Fw, False);
    }
    xwc.width = cre->width;
    xwc.height = cre->height;
    xwc.border_width = cre->border_width;

    XConfigureWindow(dpy, Event.xany.window, xwcm, &xwc);

    if(Fw)
    {
      if (cre->window != FW_W_ICON_PIXMAP(Fw) &&
	  FW_W_ICON_PIXMAP(Fw) != None)
      {
	rectangle g;

	get_icon_picture_geometry(Fw, &g);
	xwc.x = g.x;
	xwc.x = g.y;
	xwcm = cre->value_mask & (CWX | CWY);
	XConfigureWindow(dpy, FW_W_ICON_PIXMAP(Fw), xwcm, &xwc);
      }
      if(FW_W_ICON_TITLE(Fw) != None)
      {
	rectangle g;

	get_icon_title_geometry(Fw, &g);
	xwc.x = g.x;
	xwc.x = g.y;
	xwcm = cre->value_mask & (CWX | CWY);
        XConfigureWindow(dpy, FW_W_ICON_TITLE(Fw), xwcm, &xwc);
      }
    }
    if (!Fw)
      return;
  }

  if (FHaveShapeExtension && FShapesSupported)
  {
    int i;
    unsigned int u;
    Bool b;
    int boundingShaped;

    /* suppress compiler warnings w/o shape extension */
    i = 0;
    u = 0;
    b = False;

    if (FShapeQueryExtents(
	  dpy, FW_W(Fw), &boundingShaped, &i, &i, &u, &u, &b, &i, &i, &u,
	  &u))
    {
      Fw->wShaped = boundingShaped;
    }
    else
    {
      Fw->wShaped = 0;
    }
  }

  if (cre->window == FW_W(Fw))
  {
    size_borders b;
#if 0
fprintf(stderr, "cre: %d(%d) %d(%d) %d(%d)x%d(%d) w 0x%08x '%s'\n",
        cre->x, (int)(cre->value_mask & CWX),
        cre->y, (int)(cre->value_mask & CWY),
        cre->width, (int)(cre->value_mask & CWWidth),
        cre->height, (int)(cre->value_mask & CWHeight),
	(int)FW_W(Fw), (Fw->name) ? Fw->name : "");
#endif
    /* Don't modify frame_XXX fields before calling SetupWindow! */
    dx = 0;
    dy = 0;
    dw = 0;
    dh = 0;

    if (IS_SHADED(Fw) ||
        !is_function_allowed(F_MOVE, NULL, Fw, False, False))
    {
      /* forbid shaded applications to move their windows */
      cre->value_mask &= ~(CWX | CWY);
    }
    if (IS_MAXIMIZED(Fw) ||
        !is_function_allowed(F_RESIZE, NULL, Fw, False, False))
    {
      /* dont allow clients to resize maximized windows */
      cre->value_mask &= ~(CWWidth | CWHeight);
    }

    /* for restoring */
    if (cre->value_mask & CWBorderWidth)
    {
      Fw->old_bw = cre->border_width;
    }
    /* override even if border change */
    get_window_borders(Fw, &b);
    if (cre->value_mask & CWX)
      dx = cre->x - Fw->frame_g.x - b.top_left.width;
    if (cre->value_mask & CWY)
      dy = cre->y - Fw->frame_g.y - b.top_left.height;
    if (cre->value_mask & CWHeight)
    {
      if (cre->height < (WINDOW_FREAKED_OUT_SIZE - b.total_size.height))
      {
	dh = cre->height - (Fw->frame_g.height - b.total_size.height);
      }
      else
      {
	/* patch to ignore height changes to astronomically large windows
	 * (needed for XEmacs 20.4); don't care if the window is shaded here -
	 * we won't use 'height' in this case anyway */
	/* inform the buggy app about the size that *we* want */
	do_send_event = True;
      }
    }
    if (cre->value_mask & CWWidth)
    {
      if (cre->width < (WINDOW_FREAKED_OUT_SIZE - b.total_size.width))
      {
	dw = cre->width - (Fw->frame_g.width - b.total_size.width);
      }
      else
      {
	/* see above */
	do_send_event = True;
	dh = 0;
      }
    }

    /*
     * SetupWindow (x,y) are the location of the upper-left outer corner and
     * are passed directly to XMoveResizeWindow (frame).  The (width,height)
     * are the inner size of the frame.  The inner width is the same as the
     * requested client window width; the inner height is the same as the
     * requested client window height plus any title bar slop.
     */
    new_g = Fw->frame_g;
    if (IS_SHADED(Fw))
    {
      new_g.width = Fw->normal_g.width;
      new_g.height = Fw->normal_g.height;
    }
    oldnew_w = new_g.width + dw;
    oldnew_h = new_g.height + dh;
    constr_w = oldnew_w;
    constr_h = oldnew_h;
    constrain_size(
      Fw, (unsigned int *)&constr_w, (unsigned int *)&constr_h, 0, 0, 0);
    dw += (constr_w - oldnew_w);
    dh += (constr_h - oldnew_h);
    if ((cre->value_mask & CWX) && dw)
    {
      new_g.x = Fw->frame_g.x + dx;
      new_g.width = Fw->frame_g.width + dw;
    }
    else if ((cre->value_mask & CWX) && !dw)
    {
      new_g.x = Fw->frame_g.x + dx;
    }
    else if (!(cre->value_mask & CWX) && dw)
    {
      gravity_resize(Fw->hints.win_gravity, &new_g, dw, 0);
    }
    if ((cre->value_mask & CWY) && dh)
    {
      new_g.y = Fw->frame_g.y + dy;
      new_g.height = Fw->frame_g.height + dh;
    }
    else if ((cre->value_mask & CWY) && !dh)
    {
      new_g.y = Fw->frame_g.y + dy;
    }
    else if (!(cre->value_mask & CWY) && dh)
    {
      gravity_resize(Fw->hints.win_gravity, &new_g, 0, dh);
    }

    if ((cre->value_mask & CWX) || (cre->value_mask & CWY) || dw || dh)
    {
      if (IS_SHADED(Fw))
	get_shaded_geometry(Fw, &new_g, &new_g);
      frame_setup_window(
	Fw, new_g.x, new_g.y, new_g.width, new_g.height, False);
      /* make sure the window structure has the new position */
      update_absolute_geometry(Fw);
      maximize_adjust_offset(Fw);
      GNOME_SetWinArea(Fw);
    }
  }

  /*  Stacking order change requested...  */
  /*  Handle this *after* geometry changes, since we need the new
      geometry in occlusion calculations */
  if ( (cre->value_mask & CWStackMode) && !DO_IGNORE_RESTACK(Fw) )
  {
    FvwmWindow *otherwin = NULL;

    if (cre->value_mask & CWSibling)
    {
      if (XFindContext (dpy, cre->above, FvwmContext,
			(caddr_t *) &otherwin) == XCNOENT)
      {
	otherwin = NULL;
      }
      if (otherwin == Fw)
      {
	otherwin = NULL;
      }
    }
    if ((cre->detail != Above) && (cre->detail != Below))
    {
      HandleUnusualStackmodes (cre->detail, Fw, cre->window,
			       otherwin, cre->above);
    }
    /* only allow clients to restack windows within their layer */
    else if (!otherwin || compare_window_layers(otherwin, Fw) != 0)
    {
      switch (cre->detail)
      {
      case Above:
	RaiseWindow(Fw);
	break;
      case Below:
	LowerWindow(Fw);
	break;
      }
    }
    else
    {
      xwc.sibling = FW_W_FRAME(otherwin);
      xwc.stack_mode = cre->detail;
      xwcm = CWSibling | CWStackMode;
      XConfigureWindow (dpy, FW_W_FRAME(Fw), xwcm, &xwc);

      /* Maintain the condition that icon windows are stacked
	 immediately below their frame */
      /* 1. for Fw */
      xwc.sibling = FW_W_FRAME(Fw);
      xwc.stack_mode = Below;
      xwcm = CWSibling | CWStackMode;
      if (FW_W_ICON_TITLE(Fw) != None)
      {
	XConfigureWindow(dpy, FW_W_ICON_TITLE(Fw), xwcm, &xwc);
      }
      if (FW_W_ICON_PIXMAP(Fw) != None)
      {
	XConfigureWindow(dpy, FW_W_ICON_PIXMAP(Fw), xwcm, &xwc);
      }

      /* 2. for otherwin */
      if (cre->detail == Below)
      {
	xwc.sibling = FW_W_FRAME(otherwin);
	xwc.stack_mode = Below;
	xwcm = CWSibling | CWStackMode;
	if (FW_W_ICON_TITLE(otherwin) != None)
	{
	  XConfigureWindow(dpy, FW_W_ICON_TITLE(otherwin), xwcm, &xwc);
	}
	if (FW_W_ICON_PIXMAP(otherwin) != None)
	{
	  XConfigureWindow(dpy, FW_W_ICON_PIXMAP(otherwin), xwcm, &xwc);
	}
      }

      /* Maintain the stacking order ring */
      if (cre->detail == Above)
      {
	remove_window_from_stack_ring(Fw);
	add_window_to_stack_ring_after(
	  Fw, get_prev_window_in_stack_ring(otherwin));
      }
      else /* cre->detail == Below */
      {
	remove_window_from_stack_ring(Fw);
	add_window_to_stack_ring_after(Fw, otherwin);
      }

      /*
	Let the modules know that Fw changed its place
	in the stacking order
      */
      BroadcastRestackThisWindow(Fw);
    }
    /* srt (28-Apr-2001): Tk needs a ConfigureNotify event after a raise,
     * otherwise it would hang for two seconds */
    new_g = Fw->frame_g;
    do_send_event = True;
  }

#if 1
  /* This causes some ddd windows not to be drawn properly. Reverted back to
   * the old method in frame_setup_window. */
  /* domivogt (15-Oct-1999): enabled this to work around buggy apps that
   * ask for a nonsense height and expect that they really get it. */
  if (do_send_event)
  {
    SendConfigureNotify(
      Fw, Fw->frame_g.x, Fw->frame_g.y,
      new_g.width, new_g.height, cre->border_width, True);
    XSync(dpy,0);
  }
#endif
}

/***********************************************************************
 *
 *  Procedure:
 *	SendConfigureNotify - inform a client window of its geometry.
 *
 *  The input (frame) geometry will be translated to client geometry
 *  before sending.
 *
 ************************************************************************/
void SendConfigureNotify(
  FvwmWindow *tmp_win, int x, int y, unsigned int w, unsigned int h, int bw,
  Bool send_for_frame_too)
{
  XEvent client_event;
  size_borders b;

  if (!tmp_win || IS_SHADED(tmp_win))
    return;
  client_event.type = ConfigureNotify;
  client_event.xconfigure.display = dpy;
  client_event.xconfigure.event = FW_W(tmp_win);
  client_event.xconfigure.window = FW_W(tmp_win);
  get_window_borders(tmp_win, &b);
  client_event.xconfigure.x = x + b.top_left.width;
  client_event.xconfigure.y = y + b.top_left.height;
  client_event.xconfigure.width = w - b.total_size.width;
  client_event.xconfigure.height = h - b.total_size.height;
  client_event.xconfigure.border_width = bw;
  client_event.xconfigure.above = FW_W_FRAME(tmp_win);
  client_event.xconfigure.override_redirect = False;
  XSendEvent(dpy, FW_W(tmp_win), False, StructureNotifyMask, &client_event);
  if (send_for_frame_too)
  {
    /* This is for buggy tk, which waits for the real ConfigureNotify
     * on frame instead of the synthetic one on w. The geometry data
     * in the event will not be correct for the frame, but tk doesn't
     * look at that data anyway. */
    client_event.xconfigure.event = FW_W_FRAME(tmp_win);
    client_event.xconfigure.window = FW_W_FRAME(tmp_win);
    XSendEvent(dpy, FW_W_FRAME(tmp_win), False,StructureNotifyMask,&client_event);
  }

  return;
}

/***********************************************************************
 *
 *  Procedure:
 *      HandleShapeNotify - shape notification event handler
 *
 ***********************************************************************/
void HandleShapeNotify (void)
{
	DBUG("HandleShapeNotify","Routine Entered");

	if (FShapesSupported)
	{
		FShapeEvent *sev = (FShapeEvent *) &Event;

		if (!Fw)
		{
			return;
		}
		if (sev->kind != FShapeBounding)
		{
			return;
		}
		Fw->wShaped = sev->shaped;
		frame_setup_shape(
			Fw, Fw->frame_g.width,
			Fw->frame_g.height);
	}

	return;
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleVisibilityNotify - record fully visible windows for
 *      use in the RaiseLower function and the OnTop type windows.
 *
 ************************************************************************/
void HandleVisibilityNotify(void)
{
  XVisibilityEvent *vevent = (XVisibilityEvent *) &Event;

  DBUG("HandleVisibilityNotify","Routine Entered");

  if(Fw && FW_W_FRAME(Fw) == last_event_window)
  {
    if(vevent->state == VisibilityUnobscured)
    {
      SET_FULLY_VISIBLE(Fw, 1);
      SET_PARTIALLY_VISIBLE(Fw, 1);
    }
    else if (vevent->state == VisibilityPartiallyObscured)
    {
      SET_FULLY_VISIBLE(Fw, 0);
      SET_PARTIALLY_VISIBLE(Fw, 1);
    }
    else
    {
      SET_FULLY_VISIBLE(Fw, 0);
      SET_PARTIALLY_VISIBLE(Fw, 0);
    }
  }
}


/***************************************************************************
 *
 * Waits for next X or module event, fires off startup routines when startup
 * modules have finished or after a timeout if the user has specified a
 * command line module that doesn't quit or gets stuck.
 *
 ****************************************************************************/
fd_set init_fdset;

int My_XNextEvent(Display *dpy, XEvent *event)
{
  extern fd_set_size_t fd_width;
  extern int x_fd;
  fd_set in_fdset, out_fdset;
  Window targetWindow;
  int num_fd;
  int i;
  static struct timeval timeout;
  static struct timeval *timeoutP = &timeout;

  DBUG("My_XNextEvent","Routine Entered");

/* include this next bit if HandleModuleInput() gets called anywhere else
 * with queueing turned on.  Because this routine is the only place that
 * queuing is on _and_ ExecuteCommandQueue is always called immediately after
 * it is impossible for there to be anything in the queue at this point */
#if 0
  /* execute any commands queued up */
  DBUG("My_XNextEvent", "executing module comand queue");
  ExecuteCommandQueue()
#endif

  /* check for any X events already queued up.
   * Side effect: this does an XFlush if no events are queued
   * Make sure nothing between here and the select causes further X
   * requests to be sent or the select may block even though there are
   * events in the queue */
  if(XPending(dpy)) {
    DBUG("My_XNextEvent","taking care of queued up events & returning (1)");
    XNextEvent(dpy,event);
    StashEventTime(event);
    return 1;
  }

/* The SIGCHLD signal is sent every time one of our child processes
 * dies, and the SIGCHLD handler now reaps them automatically. We
 * should therefore never see a zombie */
#if 0
  DBUG("My_XNextEvent","no X events waiting - about to reap children");
  /* Zap all those zombies! */
  /* If we get to here, then there are no X events waiting to be processed.
   * Just take a moment to check for dead children. */
  ReapChildren();
#endif

  /* check for termination of all startup modules */
  if (fFvwmInStartup) {
    for(i=0;i<npipes;i++)
      if (FD_ISSET(i, &init_fdset))
        break;
    if (i == npipes || writePipes[i+1] == 0)
    {
      DBUG("My_XNextEvent", "Starting up after command lines modules\n");
      timeoutP = NULL; /* set an infinite timeout to stop ticking */
      StartupStuff(); /* This may cause X requests to be sent */
      return 0; /* so return without select()ing */
    }
  }

  /* Some signals can interrupt us while we wait for any action
   * on our descriptors. While some of these signals may be asking
   * fvwm to die, some might be harmless. Harmless interruptions
   * mean we have to start waiting all over again ... */
  do
  {
    int ms;
    Bool is_waiting_for_scheduled_command = False;
    static struct timeval *old_timeoutP = NULL;

    /* The timeouts become undefined whenever the select returns, and so
     * we have to reinitialise them */
    ms = squeue_get_next_ms();
    if (ms == 0)
    {
      /* run scheduled commands */
      squeue_execute();
      ms = squeue_get_next_ms();
      /* should not happen anyway. get_next_schedule_queue_ms() can't return 0
       * after a call to execute_schedule_queue(). */
      if (ms == 0)
      {
	ms = 1;
      }
    }
    if (ms < 0)
    {
      timeout.tv_sec = 42;
      timeout.tv_usec = 0;
    }
    else
    {
      /* scheduled commands are pending - don't wait too long */
      timeout.tv_sec = ms / 1000;
      timeout.tv_usec = 1000 * (ms % 1000);
      old_timeoutP = timeoutP;
      timeoutP = &timeout;
      is_waiting_for_scheduled_command = True;
    }

    FD_ZERO(&in_fdset);
    FD_ZERO(&out_fdset);
    FD_SET(x_fd, &in_fdset);

    /* nothing is done here if fvwm was compiled without session support */
    if (sm_fd >= 0)
      FD_SET(sm_fd, &in_fdset);

    for(i=0; i<npipes; i++) {
      if(readPipes[i]>=0)
        FD_SET(readPipes[i], &in_fdset);
      if (!FQUEUE_IS_EMPTY(&pipeQueue[i]))
        FD_SET(writePipes[i], &out_fdset);
    }

    DBUG("My_XNextEvent","waiting for module input/output");
    num_fd = fvwmSelect(fd_width, &in_fdset, &out_fdset, 0, timeoutP);
    if (is_waiting_for_scheduled_command)
    {
      timeoutP = old_timeoutP;
    }

    /* Express route out of FVWM ... */
    if ( isTerminated ) return 0;
  }
  while (num_fd < 0);

  if (num_fd > 0) {

    /* Check for module input. */
    for (i=0; i<npipes; i++) {
      if ((readPipes[i] >= 0) && FD_ISSET(readPipes[i], &in_fdset)) {
        if (read(readPipes[i], &targetWindow, sizeof(Window)) > 0) {
          DBUG("My_XNextEvent","calling HandleModuleInput");
          /* Add one module message to the queue */
          HandleModuleInput(targetWindow, i, NULL, True);
        } else {
          DBUG("My_XNextEvent","calling KillModule");
          KillModule(i);
        }
      }
      if ((writePipes[i] >= 0) && FD_ISSET(writePipes[i], &out_fdset)) {
        DBUG("My_XNextEvent","calling FlushMessageQueue");
        FlushMessageQueue(i);
      }
    }

    /* execute any commands queued up */
    DBUG("My_XNextEvent", "executing module comand queue");
    ExecuteCommandQueue();

    /* nothing is done here if fvwm was compiled without session support */
    if ((sm_fd >= 0) && (FD_ISSET(sm_fd, &in_fdset)))
      ProcessICEMsgs();

  } else {
    /* select has timed out, things must have calmed down so let's decorate */
    if (fFvwmInStartup) {
      fvwm_msg(ERR, "My_XNextEvent",
               "Some command line modules have not quit, "
	       "Starting up after timeout.\n");
      StartupStuff();
      timeoutP = NULL; /* set an infinite timeout to stop ticking */
      reset_style_changes();
      Scr.flags.do_need_window_update = 0;
    }
    /* run scheduled commands if necessary */
    squeue_execute();
  }

  /* check for X events again, rather than return 0 and get called again */
  if(XPending(dpy)) {
    DBUG("My_XNextEvent","taking care of queued up events & returning (2)");
    XNextEvent(dpy,event);
    StashEventTime(event);
    return 1;
  }

  DBUG("My_XNextEvent","leaving My_XNextEvent");
  return 0;
}

/*
** Procedure:
**   InitEventHandlerJumpTable
*/
void InitEventHandlerJumpTable(void)
{
  int i;

  for (i=0; i<LASTEvent; i++)
  {
    EventHandlerJumpTable[i] = NULL;
  }
  EventHandlerJumpTable[Expose] =           HandleExpose;
  EventHandlerJumpTable[DestroyNotify] =    HandleDestroyNotify;
  EventHandlerJumpTable[MapRequest] =       HandleMapRequest;
  EventHandlerJumpTable[MapNotify] =        HandleMapNotify;
  EventHandlerJumpTable[UnmapNotify] =      HandleUnmapNotify;
  EventHandlerJumpTable[ButtonPress] =      HandleButtonPress;
  EventHandlerJumpTable[EnterNotify] =      HandleEnterNotify;
  EventHandlerJumpTable[LeaveNotify] =      HandleLeaveNotify;
  EventHandlerJumpTable[FocusIn] =          HandleFocusIn;
  EventHandlerJumpTable[ConfigureRequest] = HandleConfigureRequest;
  EventHandlerJumpTable[ClientMessage] =    HandleClientMessage;
  EventHandlerJumpTable[PropertyNotify] =   HandlePropertyNotify;
  EventHandlerJumpTable[KeyPress] =         HandleKeyPress;
  EventHandlerJumpTable[VisibilityNotify] = HandleVisibilityNotify;
  EventHandlerJumpTable[ColormapNotify] =   HandleColormapNotify;
  if (FShapesSupported)
    EventHandlerJumpTable[FShapeEventBase+FShapeNotify] = HandleShapeNotify;
  EventHandlerJumpTable[SelectionClear]   = HandleSelectionClear;
  EventHandlerJumpTable[SelectionRequest] = HandleSelectionRequest;
  EventHandlerJumpTable[ReparentNotify] = HandleReparentNotify;
  STROKE_CODE(EventHandlerJumpTable[ButtonRelease] =    HandleButtonRelease);
  STROKE_CODE(EventHandlerJumpTable[MotionNotify] =     HandleMotionNotify);
#ifdef MOUSE_DROPPINGS
  STROKE_CODE(stroke_init(dpy,DefaultRootWindow(dpy)));
#else /* no MOUSE_DROPPINGS */
  STROKE_CODE(stroke_init());
#endif /* MOUSE_DROPPINGS */
}

/***********************************************************************
 *
 *  Procedure:
 *	DispatchEvent - handle a single X event stored in global var Event
 *
 ************************************************************************/
void DispatchEvent(Bool preserve_Fw)
{
  Window w = Event.xany.window;
  FvwmWindow *s_Fw = NULL;

  DBUG("DispatchEvent","Routine Entered");

  if (preserve_Fw)
    s_Fw = Fw;
  StashEventTime(&Event);

  XFlush(dpy);
  if (XFindContext (dpy, w, FvwmContext, (caddr_t *) &Fw) == XCNOENT)
  {
    Fw = NULL;
  }
  last_event_type = Event.type;
  last_event_window = w;

  if (EventHandlerJumpTable[Event.type])
  {
    (*EventHandlerJumpTable[Event.type])();
  }

#ifdef C_ALLOCA
  /* If we're using the C version of alloca, see if anything needs to be
   * freed up.
   */
  alloca(0);
#endif

  if (preserve_Fw)
  {
    /* Just to be safe, check if the saved window still exists.  Since this is
     * only used with Expose events we should be safe anyway, but a bit of
     * security does not hurt. */
    if (check_if_fvwm_window_exists(s_Fw))
      Fw = s_Fw;
    else
      Fw = NULL;
  }

  DBUG("DispatchEvent","Leaving Routine");
  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleEvents - handle X events
 *
 ************************************************************************/
void HandleEvents(void)
{
  DBUG("HandleEvents","Routine Entered");
  STROKE_CODE(send_motion = FALSE);
  while ( !isTerminated )
  {
    last_event_type = 0;
    if (Scr.flags.is_window_scheduled_for_destroy)
    {
      destroy_scheduled_windows();
    }
    if (Scr.flags.do_need_window_update)
    {
      flush_window_updates();
    }
    if (Scr.flags.do_need_style_list_update)
    {
      simplify_style_list();
    }
    if(My_XNextEvent(dpy, &Event))
    {
      DispatchEvent(False);
    }
  }
}

/***********************************************************************
 *
 *  Procedure:
 *	Find the Fvwm context for the Event.
 *
 ************************************************************************/
int GetContext(FvwmWindow *t, XEvent *e, Window *w)
{
  int Context,i;

  Context = C_NO_CONTEXT;
  if (e->type == KeyPress && e->xkey.window == Scr.Root &&
      e->xkey.subwindow != None)
  {
    /* Translate root coordinates into subwindow coordinates.  Necessary for
     * key bindings that work over unfocused windows. */
    e->xkey.window = e->xkey.subwindow;
    XTranslateCoordinates(
      dpy, Scr.Root, e->xkey.subwindow, e->xkey.x, e->xkey.y, &(e->xkey.x),
      &(e->xkey.y), &(e->xkey.subwindow));
    XFindContext(dpy, e->xkey.window, FvwmContext, (caddr_t *) &t);
    Fw = t;
  }
  if (e->type == ButtonPress && t && e->xkey.window == FW_W_FRAME(t) &&
      e->xkey.subwindow != None)
  {
    /* Translate frame coordinates into subwindow coordinates. */
    e->xkey.window = e->xkey.subwindow;
    XTranslateCoordinates(
      dpy, FW_W_FRAME(t), e->xkey.subwindow, e->xkey.x, e->xkey.y, &(e->xkey.x),
      &(e->xkey.y), &(e->xkey.subwindow));
    if (e->xkey.window == FW_W_PARENT(t))
    {
      e->xkey.window = e->xkey.subwindow;
      XTranslateCoordinates(
	dpy, FW_W_PARENT(t), e->xkey.subwindow, e->xkey.x, e->xkey.y,
	&(e->xkey.x), &(e->xkey.y), &(e->xkey.subwindow));
    }
  }
  if (!t)
  {
    return C_ROOT;
  }

  if (e->type == KeyPress && e->xkey.window == FW_W_FRAME(t) &&
      e->xkey.subwindow == t->decor_w)
  {
    /* We can't get keyboard events on the decor_w directly because it is a
     * sibling of the parent window which gets all keyboard input. So we have
     * to grab keys on the frame and then translate the coordinates to find out
     * in which subwindow of the decor_w the event occured. */
    e->xkey.window = e->xkey.subwindow;
    XTranslateCoordinates(dpy, FW_W_FRAME(t), t->decor_w, e->xkey.x, e->xkey.y,
			  &JunkX, &JunkY, &(e->xkey.subwindow));
  }
  *w= e->xany.window;

  if (*w == Scr.NoFocusWin)
  {
    return C_ROOT;
  }
  if (e->type == KeyPress && e->xkey.window == FW_W_FRAME(t) &&
      e->xkey.subwindow == t->decor_w)
  {
    /* We can't get keyboard events on the decor_w directly because it is a
     * sibling of the parent window which gets all keyboard input. So we have
     * to grab keys on the frame and then translate the coordinates to find out
     * in which subwindow of the decor_w the event occured. */
    e->xkey.window = e->xkey.subwindow;
    XTranslateCoordinates(dpy, FW_W_FRAME(t), t->decor_w, e->xkey.x, e->xkey.y,
			  &JunkX, &JunkY, &(e->xkey.subwindow));
  }
  *w= e->xany.window;

  if (*w == Scr.NoFocusWin)
  {
    return C_ROOT;
  }
  if (e->xkey.subwindow != None &&
      (e->xkey.window == t->decor_w || e->xkey.window == FW_W_PARENT(t)))
  {
    *w = e->xkey.subwindow;
  }
  else
  {
    e->xkey.subwindow = None;
  }
  if (*w == Scr.Root)
  {
    return C_ROOT;
  }
  if (t)
  {
    if (*w == FW_W_TITLE(t))
      Context = C_TITLE;
    else if (Scr.EwmhDesktop &&
	     (*w == FW_W(Scr.EwmhDesktop) ||
	      *w == FW_W_PARENT(Scr.EwmhDesktop) ||
	      *w == FW_W_FRAME(Scr.EwmhDesktop)))
      Context = C_EWMH_DESKTOP;
    else if (*w == FW_W(t) || *w == FW_W_PARENT(t) || *w == FW_W_FRAME(t))
      Context = C_WINDOW;
    else if (*w == FW_W_ICON_TITLE(t) || *w == FW_W_ICON_PIXMAP(t))
      Context = C_ICON;
    else if (*w == t->decor_w)
      Context = C_SIDEBAR;
    else if (*w == FW_W_CORNER(t, 0))
    {
      Context = C_F_TOPLEFT;
      Button = 0;
    }
    else if (*w == FW_W_CORNER(t, 1))
    {
      Context = C_F_TOPRIGHT;
      Button = 1;
    }
    else if (*w == FW_W_CORNER(t, 2))
    {
      Context = C_F_BOTTOMLEFT;
      Button = 2;
    }
    else if (*w == FW_W_CORNER(t, 3))
    {
      Context = C_F_BOTTOMRIGHT;
      Button = 3;
    }
    else if (*w == FW_W_SIDE(t, 0))
    {
      Context = C_SB_TOP;
      Button = 0;
    }
    else if (*w == FW_W_SIDE(t, 1))
    {
      Context = C_SB_RIGHT;
      Button = 1;
    }
    else if (*w == FW_W_SIDE(t, 2))
    {
      Context = C_SB_BOTTOM;
      Button = 2;
    }
    else if (*w == FW_W_SIDE(t, 3))
    {
      Context = C_SB_LEFT;
      Button = 3;
    }
    else
    {
      for (i = 0; i < NUMBER_OF_BUTTONS; i++)
      {
        if (*w == FW_W_BUTTON(t, i))
        {
          if ((!(i & 1) && i / 2 < Scr.nr_left_buttons) ||
              ( (i & 1) && i / 2 < Scr.nr_right_buttons))
          {
            Context = (1 << i) * C_L1;
            Button = i;
            break;
          }
        }
      }
    } /* else */
  } /* if (t) */
  return Context;
}


/**************************************************************************
 *
 * Removes expose events for a specific window from the queue
 *
 *************************************************************************/
int flush_expose (Window w)
{
  XEvent dummy;
  int i=0;

  while (XCheckTypedWindowEvent(dpy, w, Expose, &dummy))
    i++;
  return i;
}

/* same as above, but merges the expose rectangles into a single big one */
int flush_accumulate_expose(Window w, XEvent *e)
{
  XEvent dummy;
  int i = 0;
  int x1 = e->xexpose.x;
  int y1 = e->xexpose.y;
  int x2 = x1 + e->xexpose.width;
  int y2 = y1 + e->xexpose.height;

  while (XCheckTypedWindowEvent(dpy, w, Expose, &dummy))
  {
    x1 = min(x1, dummy.xexpose.x);
    y1 = min(y1, dummy.xexpose.y);
    x2 = max(x2, dummy.xexpose.x + dummy.xexpose.width);
    y2 = max(y2, dummy.xexpose.y + dummy.xexpose.height);
    i++;
  }
  e->xexpose.x = x1;
  e->xexpose.y = y1;
  e->xexpose.width = x2 - x1;
  e->xexpose.height = y2 - y1;

  return i;
}


/**************************************************************************
 *
 * Removes all expose events from the queue and does the necessary redraws
 *
 *************************************************************************/
void handle_all_expose(void)
{
  XEvent old_event;

  memcpy(&old_event, &Event, sizeof(XEvent));
  XPending(dpy);
  while (XCheckMaskEvent(dpy, ExposureMask, &Event))
  {
    /* note: handling Expose events will never modify the global Fw */
    DispatchEvent(True);
  }
  memcpy(&Event, &old_event, sizeof(XEvent));

  return;
}


/****************************************************************************
 *
 * Records the time of the last processed event. Used in XSetInputFocus
 *
 ****************************************************************************/
Bool StashEventTime (XEvent *ev)
{
  Time NewTimestamp = CurrentTime;

  switch (ev->type)
  {
  case KeyPress:
  case KeyRelease:
    NewTimestamp = ev->xkey.time;
    break;
  case ButtonPress:
  case ButtonRelease:
    NewTimestamp = ev->xbutton.time;
    break;
  case MotionNotify:
    NewTimestamp = ev->xmotion.time;
    break;
  case EnterNotify:
  case LeaveNotify:
    NewTimestamp = ev->xcrossing.time;
    break;
  case PropertyNotify:
    NewTimestamp = ev->xproperty.time;
    break;
  case SelectionClear:
    NewTimestamp = ev->xselectionclear.time;
    break;
  case SelectionRequest:
    NewTimestamp = ev->xselectionrequest.time;
    break;
  case SelectionNotify:
    NewTimestamp = ev->xselection.time;
    break;
  default:
    return False;
  }
  /* Only update if the new timestamp is later than the old one, or
   * if the new one is from a time at least 30 seconds earlier than the
   * old one (in which case the system clock may have changed) */
  if (NewTimestamp > lastTimestamp ||
      lastTimestamp - NewTimestamp > CLOCK_SKEW_MS)
    lastTimestamp = NewTimestamp;
  return True;
}

/* CoerceEnterNotifyOnCurrentWindow()
 * Pretends to get a HandleEnterNotify on the
 * window that the pointer currently is in so that
 * the focus gets set correctly from the beginning
 * Note that this presently only works if the current
 * window is not click_to_focus;  I think that
 * that behaviour is correct and desirable. --11/08/97 gjb */
void CoerceEnterNotifyOnCurrentWindow(void)
{
  extern FvwmWindow *Fw; /* from events.c */
  Window child, root;
  int root_x, root_y;
  int win_x, win_y;
  Bool f = XQueryPointer(dpy, Scr.Root, &root,
                         &child, &root_x, &root_y, &win_x, &win_y, &JunkMask);

  if (f && child != None) {
    Event.xany.window = child;
    if (XFindContext(dpy, child, FvwmContext, (caddr_t *) &Fw) == XCNOENT)
      Fw = NULL;
    HandleEnterNotify();
    Fw = None;
  }
}

/* This function discards all queued up ButtonPress, ButtonRelease and
 * ButtonMotion events. */
int discard_events(long event_mask)
{
  XEvent e;
  int count;

  XSync(dpy, 0);
  for (count = 0; XCheckMaskEvent(dpy, event_mask, &e); count++)
  {
    StashEventTime(&e);
  }

  return count;
}

/* This function discards all queued up ButtonPress, ButtonRelease and
 * ButtonMotion events. */
int discard_window_events(Window w, long event_mask)
{
  XEvent e;
  int count;

  XSync(dpy, 0);
  for (count = 0; XCheckWindowEvent(dpy, w, event_mask, &e); count++)
  {
    StashEventTime(&e);
  }

  return count;
}

/* Similar function for certain types of PropertyNotify. */
int flush_property_notify(Atom atom, Window w)
{
  XEvent e;
  int count;

  XSync(dpy, 0);
  for (count = 0; XCheckTypedWindowEvent(dpy, w, PropertyNotify, &e);
       count++)
  {
    if (e.xproperty.atom != atom)
    {
      XPutBackEvent(dpy, &e);
      break;
    }
  }

  return count;
}

/***************************************************************************
 *
 * Wait for all mouse buttons to be released
 * This can ease some confusion on the part of the user sometimes
 *
 * Discard superflous button events during this wait period.
 *
 ***************************************************************************/
void WaitForButtonsUp(Bool do_handle_expose)
{
  unsigned int mask;
  long evmask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
    KeyPressMask|KeyReleaseMask;

  if (do_handle_expose)
    evmask |= ExposureMask;
  MyXGrabServer(dpy);
  mask = DEFAULT_ALL_BUTTONS_MASK;
  while (mask & (DEFAULT_ALL_BUTTONS_MASK))
  {
    /* handle expose events */
    XAllowEvents(dpy, SyncPointer, CurrentTime);
    if (XCheckMaskEvent(dpy, evmask, &Event))
    {
      switch (Event.type)
      {
      case ButtonReleaseMask:
	mask = Event.xbutton.state;
	break;
      case Expose:
	/* note: handling Expose events will never modify the global Fw */
	DispatchEvent(True);
	break;
      default:
	break;
      }
    }
    else
    {
      /* although this should never happen, a bit of additional safety does not
       * hurt. */
      if (XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild, &JunkX, &JunkY,
			&JunkX, &JunkY, &mask) == False)
      {
	/* pointer is on a different screen - that's okay here */
      }
    }
  }
  MyXUngrabServer(dpy);

  return;
}

void CMD_XSync(F_CMD_ARGS)
{
	XSync(dpy, 0);

	return;
}

void sync_server(int toggle)
{
	static Bool synced = False;

	if (toggle == -1)
	{
		toggle = (synced == False);
	}
	if (toggle == 1)
	{
		synced = True;
	}
	else
	{
		synced = False;
	}
	XSynchronize(dpy, synced);

	return;
}

void CMD_XSynchronize(F_CMD_ARGS)
{
	int toggle;

	toggle = ParseToggleArgument(action, NULL, -1, 0);
	sync_server(toggle);

	return;
}
