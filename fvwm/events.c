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
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */
#include "module_interface.h"
#include "session.h"
#include "borders.h"
#include "colormaps.h"
#include "add_window.h"
#include "icccm2.h"
#include "icons.h"
#include "gnome.h"
#include "update.h"
#include "style.h"
#include "stack.h"
#include "geometry.h"
#include "focus.h"
#include "move_resize.h"
#include "virtual.h"
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
FvwmWindow *Tmp_win = NULL;		/* the current fvwm window */

int last_event_type=0;
static Window last_event_window=0;
Time lastTimestamp = CurrentTime;	/* until Xlib does this for us */


STROKE_CODE(static int send_motion;)
STROKE_CODE(static char sequence[MAX_SEQUENCE+1];)

#ifdef SHAPE
extern int ShapeEventBase;
void HandleShapeNotify(void);
#endif /* SHAPE */

Window PressedW;

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
  FvwmWindow *ffw_old = Scr.Focus;
  static Window last_focus_w = None;
  static Window last_focus_fw = None;
  static Bool is_never_focused = True;

  DBUG("HandleFocusIn","Routine Entered");

  /* This is a hack to make the PointerKey command work */
  if (Event.xfocus.detail != NotifyPointer)
  /**/
    w= Event.xany.window;
  while(XCheckTypedEvent(dpy,FocusIn,&d))
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
  if (XFindContext (dpy, w, FvwmContext, (caddr_t *) &Tmp_win) == XCNOENT)
  {
    Tmp_win = NULL;
  }

  if (!Tmp_win)
  {
    if (w != Scr.NoFocusWin)
    {
      Scr.UnknownWinFocused = w;
      focus_w = w;
    }
    else
    {
      DrawDecorations(Scr.Hilite, DRAW_ALL, False, True, None);
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
    fc = GetColor("White");
    bc = GetColor("Black");
  }
  else if (Tmp_win != Scr.Hilite
	   /* domivogt (16-May-2000): This check is necessary to force sending
	    * a M_FOCUS_CHANGE packet after an unmanaged window was focused.
	    * Otherwise fvwm would believe that Scr.Hilite was still focused and
	    * not send any info to the modules. */
	   || last_focus_fw == None)
  {
    DrawDecorations(Tmp_win, DRAW_ALL, True, True, None);
    focus_w = Tmp_win->w;
    focus_fw = Tmp_win->frame;
    fc = Tmp_win->hicolors.fore;
    bc = Tmp_win->hicolors.back;
    Scr.Focus = Tmp_win;
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
      focus_w != last_focus_w || focus_fw != last_focus_fw)
  {
    BroadcastPacket(M_FOCUS_CHANGE, 5, focus_w, focus_fw,
                    (unsigned long)IsLastFocusSetByMouse(), fc, bc);
    last_focus_w = focus_w;
    last_focus_fw = focus_fw;
    is_never_focused = False;
  }
  if (Scr.Focus != ffw_old)
  {
    focus_grab_buttons(Scr.Focus, True);
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

  DBUG("HandleKeyPress","Routine Entered");

  Context = GetContext(Tmp_win, &Event, &PressedW);
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
    ButtonWindow = Tmp_win;
    old_execute_function(action, Tmp_win, &Event, Context, -1, 0, NULL);
    ButtonWindow = NULL;
    return;
  }

  /* if we get here, no function key was bound to the key.  Send it
   * to the client if it was in a window we know about.
   */
  if (Scr.Focus && Event.xkey.window != Scr.Focus->w)
  {
    Event.xkey.window = Scr.Focus->w;
    XSendEvent(dpy, Scr.Focus->w, False, KeyPressMask, &Event);
  }
  else if (Tmp_win && Event.xkey.window != Tmp_win->w)
  {
    Event.xkey.window = Tmp_win->w;
    XSendEvent(dpy, Tmp_win->w, False, KeyPressMask, &Event);
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
  XTextProperty text_prop;
  Bool OnThisPage = False;
  Bool was_size_inc_set;
  int old_wmhints_flags;
  int old_width_inc;
  int old_height_inc;
  int old_base_width;
  int old_base_height;

#ifdef I18N_MB
  Atom actual = None;
  int actual_format;
  unsigned long nitems, bytesafter;
  char *prop = NULL;
  char **list;
  int num;
#endif

  DBUG("HandlePropertyNotify","Routine Entered");

  if ((!Tmp_win)||
      (XGetGeometry(dpy, Tmp_win->w, &JunkRoot, &JunkX, &JunkY,
		    &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0))
    return;

  /*
   * Make sure at least part of window is on this page
   * before giving it focus...
   */
  OnThisPage = IsRectangleOnThisPage(&(Tmp_win->frame_g), Tmp_win->Desk);

  switch (Event.xproperty.atom)
    {
    case XA_WM_TRANSIENT_FOR:
      {
        if(XGetTransientForHint(dpy, Tmp_win->w, &Tmp_win->transientfor))
        {
	  SET_TRANSIENT(Tmp_win, 1);
	  RaiseWindow(Tmp_win);
        }
        else
        {
	  SET_TRANSIENT(Tmp_win, 0);
        }
      }
      break;

    case XA_WM_NAME:
#ifdef I18N_MB
      if (XGetWindowProperty (dpy, Tmp_win->w, Event.xproperty.atom, 0L,
			      MAX_WINDOW_NAME_LEN, False, AnyPropertyType,
			      &actual, &actual_format, &nitems, &bytesafter,
			      (unsigned char **) &prop) != Success ||
	  actual == None)
	return;
      if (prop) {
        if (actual == XA_STRING) {
          /* STRING encoding, use this as it is */
          free_window_names (Tmp_win, True, False);
          Tmp_win->name = prop;
          Tmp_win->name_list = NULL;
        } else {
          /* not STRING encoding, try to convert */
          text_prop.value = prop;
          text_prop.encoding = actual;
          text_prop.format = actual_format;
          text_prop.nitems = nitems;
          if (
	    XmbTextPropertyToTextList(dpy, &text_prop, &list, &num) >= Success
	    && num > 0 && *list) {
            /* XXX: does not consider the conversion is REALLY succeeded */
            XFree(prop); /* return of XGetWindowProperty() */
            free_window_names (Tmp_win, True, False);
            Tmp_win->name = *list;
            Tmp_win->name_list = list;
          } else {
            if (list) XFreeStringList(list);
            XFree(prop); /* return of XGetWindowProperty() */
            if (!XGetWMName(dpy, Tmp_win->w, &text_prop))
              return; /* why cannot read... */
            free_window_names (Tmp_win, True, False);
            Tmp_win->name = (char *)text_prop.value;
            Tmp_win->name_list = NULL;
          }
        }
      } else {
        /* XXX: fallback to original behavior, is it needed ? */
        if (!XGetWMName(dpy, Tmp_win->w, &text_prop))
	  return;
        free_window_names (Tmp_win, True, False);
        Tmp_win->name = (char *)text_prop.value;
        Tmp_win->name_list = NULL;
      }
#else
      if (!XGetWMName(dpy, Tmp_win->w, &text_prop))
	return;
      free_window_names (Tmp_win, True, False);
      Tmp_win->name = (char *)text_prop.value;
      if (Tmp_win->name && strlen(Tmp_win->name) > MAX_WINDOW_NAME_LEN)
	/* limit to prevent hanging X server */
	Tmp_win->name[MAX_WINDOW_NAME_LEN] = 0;
#endif

      SET_NAME_CHANGED(Tmp_win, 1);

      if (Tmp_win->name == NULL)
        Tmp_win->name = NoName;
      BroadcastName(M_WINDOW_NAME,Tmp_win->w,Tmp_win->frame,
		    (unsigned long)Tmp_win,Tmp_win->name);

      /* fix the name in the title bar */
      if(!IS_ICONIFIED(Tmp_win))
	DrawDecorations(
	  Tmp_win, DRAW_TITLE, (Scr.Hilite == Tmp_win), True, None);

      /*
       * if the icon name is NoName, set the name of the icon to be
       * the same as the window
       */
      if (Tmp_win->icon_name == NoName)
	{
	  Tmp_win->icon_name = Tmp_win->name;
	  BroadcastName(M_ICON_NAME,Tmp_win->w,Tmp_win->frame,
			(unsigned long)Tmp_win,Tmp_win->icon_name);
	  RedoIconName(Tmp_win);
	}
      break;

    case XA_WM_ICON_NAME:
#ifdef I18N_MB
      if (XGetWindowProperty (dpy, Tmp_win->w, Event.xproperty.atom, 0L,
			      MAX_ICON_NAME_LEN, False, AnyPropertyType,
			      &actual, &actual_format, &nitems, &bytesafter,
			      (unsigned char **) &prop) != Success ||
	  actual == None)
	return;
      if (prop) {
        if (actual == XA_STRING) {
          /* STRING encoding, use this as it is */
          free_window_names (Tmp_win, False, True);
          Tmp_win->icon_name = prop;
          Tmp_win->icon_name_list = NULL;
        } else {
          /* not STRING encoding, try to convert */
          text_prop.value = prop;
          text_prop.encoding = actual;
          text_prop.format = actual_format;
          text_prop.nitems = nitems;
          if (XmbTextPropertyToTextList(dpy, &text_prop, &list, &num) >= Success
              && num > 0 && *list) {
            /* XXX: does not consider the conversion is REALLY succeeded */
            XFree(prop); /* return of XGetWindowProperty() */
            free_window_names (Tmp_win, False, True);
            Tmp_win->icon_name = *list;
            Tmp_win->icon_name_list = list;
          } else {
            if (list) XFreeStringList(list);
            XFree(prop); /* return of XGetWindowProperty() */
            if (!XGetWMIconName (dpy, Tmp_win->w, &text_prop))
              return; /* why cannot read... */
            free_window_names (Tmp_win, False, True);
            Tmp_win->icon_name = (char *)text_prop.value;
            Tmp_win->icon_name_list = NULL;
          }
        }
      } else {
        /* XXX: fallback to original behavior, is it needed ? */
        if (!XGetWMIconName(dpy, Tmp_win->w, &text_prop))
          return;
        free_window_names (Tmp_win, False, True);
        Tmp_win->icon_name = (char *)text_prop.value;
        Tmp_win->icon_name_list = NULL;
      }
#else
      if (!XGetWMIconName (dpy, Tmp_win->w, &text_prop))
	return;
      free_window_names (Tmp_win, False, True);
      Tmp_win->icon_name = (char *) text_prop.value;
      if (Tmp_win->icon_name && strlen(Tmp_win->icon_name) >
	  MAX_ICON_NAME_LEN)
	/* limit to prevent hanging X server */
	Tmp_win->icon_name[MAX_ICON_NAME_LEN] = 0;
#endif
      if (Tmp_win->icon_name == NULL)
        Tmp_win->icon_name = NoName;
      BroadcastName(M_ICON_NAME,Tmp_win->w,Tmp_win->frame,
		    (unsigned long)Tmp_win,Tmp_win->icon_name);
      RedoIconName(Tmp_win);
      break;

    case XA_WM_HINTS:
      /* clasen@mathematik.uni-freiburg.de - 02/01/1998 - new -
	 the urgency flag is an ICCCM 2.0 addition to the WM_HINTS. */
      old_wmhints_flags = 0;
      if (Tmp_win->wmhints) {
        old_wmhints_flags = Tmp_win->wmhints->flags;
	XFree ((char *) Tmp_win->wmhints);
      }
      Tmp_win->wmhints = XGetWMHints(dpy, Event.xany.window);

      if(Tmp_win->wmhints == NULL)
	return;

      /*
       * rebuild icon if the client either provides an icon
       * pixmap or window or has reset the hints to `no icon'.
       */
      if ((Tmp_win->wmhints->flags & (IconPixmapHint|IconWindowHint)) ||
          ((old_wmhints_flags & (IconPixmapHint|IconWindowHint)) !=
	   (Tmp_win->wmhints->flags & (IconPixmapHint|IconWindowHint))))
	{
	  if(Tmp_win->icon_bitmap_file == Scr.DefaultIcon)
	    Tmp_win->icon_bitmap_file = NULL;
          if(!Tmp_win->icon_bitmap_file &&
             !(Tmp_win->wmhints->flags&(IconPixmapHint|IconWindowHint)))
	  {
	    Tmp_win->icon_bitmap_file =
	      (Scr.DefaultIcon) ? strdup(Scr.DefaultIcon) : NULL;
	  }

	  if (!IS_ICON_SUPPRESSED(Tmp_win) ||
	      (Tmp_win->wmhints->flags & IconWindowHint))
	    {
	      if (Tmp_win->icon_w)
		XDestroyWindow(dpy,Tmp_win->icon_w);
	      XDeleteContext(dpy, Tmp_win->icon_w, FvwmContext);
	      if(IS_ICON_OURS(Tmp_win))
		{
		  if(Tmp_win->icon_pixmap_w != None)
		    {
		      XDestroyWindow(dpy,Tmp_win->icon_pixmap_w);
		      XDeleteContext(dpy, Tmp_win->icon_pixmap_w, FvwmContext);
		    }
		}
	      else
		XUnmapWindow(dpy,Tmp_win->icon_pixmap_w);
	    }
	  Tmp_win->icon_w = None;
	  Tmp_win->icon_pixmap_w = None;
	  Tmp_win->iconPixmap = (Window)NULL;
	  if(IS_ICONIFIED(Tmp_win))
	    {
	      SET_ICONIFIED(Tmp_win, 0);
	      SET_ICON_UNMAPPED(Tmp_win, 0);
	      CreateIconWindow(Tmp_win,
			       Tmp_win->icon_g.x,Tmp_win->icon_g.y);
	      BroadcastPacket(M_ICONIFY, 7,
			      Tmp_win->w, Tmp_win->frame,
			      (unsigned long)Tmp_win,
			      Tmp_win->icon_g.x, Tmp_win->icon_g.y,
			      Tmp_win->icon_g.width, Tmp_win->icon_g.height);
	      /* domivogt (15-Sep-1999): BroadcastConfig informs modules of the
	       * configuration change including the iconified flag. So this
	       * flag must be set here. I'm not sure if the two calls of the
	       * SET_ICONIFIED macro after BroadcastConfig are necessary, but
	       * since it's only minimal overhead I prefer to be on the safe
	       * side. */
	      SET_ICONIFIED(Tmp_win, 1);
	      BroadcastConfig(M_CONFIGURE_WINDOW, Tmp_win);
	      SET_ICONIFIED(Tmp_win, 0);

	      if (!IS_ICON_SUPPRESSED(Tmp_win))
		{
		  LowerWindow(Tmp_win);
		  AutoPlaceIcon(Tmp_win);
		  if(Tmp_win->Desk == Scr.CurrentDesk)
		    {
		      if(Tmp_win->icon_w)
			XMapWindow(dpy, Tmp_win->icon_w);
		      if(Tmp_win->icon_pixmap_w != None)
			XMapWindow(dpy, Tmp_win->icon_pixmap_w);
		    }
		}
	      SET_ICONIFIED(Tmp_win, 1);
	      DrawIconWindow(Tmp_win);
	    }
	}

      /* clasen@mathematik.uni-freiburg.de - 02/01/1998 - new -
	 the urgency flag is an ICCCM 2.0 addition to the WM_HINTS.
	 Treat urgency changes by calling user-settable functions.
	 These could e.g. deiconify and raise the window or temporarily
	 change the decor. */
      if (!(old_wmhints_flags & XUrgencyHint) &&
	  (Tmp_win->wmhints->flags & XUrgencyHint))
	{
	  old_execute_function(
	    "Function UrgencyFunc", Tmp_win, &Event, C_WINDOW, -1, 0, NULL);
	}

      if ((old_wmhints_flags & XUrgencyHint) &&
	  !(Tmp_win->wmhints->flags & XUrgencyHint))
	{
	  old_execute_function(
	    "Function UrgencyDoneFunc", Tmp_win, &Event, C_WINDOW, -1, 0, NULL);
	}
      break;
    case XA_WM_NORMAL_HINTS:
      was_size_inc_set = IS_SIZE_INC_SET(Tmp_win);
      old_width_inc = Tmp_win->hints.width_inc;
      old_height_inc = Tmp_win->hints.height_inc;
      old_base_width = Tmp_win->hints.base_width;
      old_base_height = Tmp_win->hints.base_height;
      GetWindowSizeHints(Tmp_win);
      if (old_width_inc != Tmp_win->hints.width_inc ||
	  old_height_inc != Tmp_win->hints.height_inc)
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
          /* we have to resize the unmaximized window to keep the size in
           * resize increments constant */
          units_w = Tmp_win->normal_g.width - 2 * Tmp_win->boundary_width -
            old_base_width;
          units_h = Tmp_win->normal_g.height - Tmp_win->title_g.height -
            2 * Tmp_win->boundary_width - old_base_height;
          units_w /= old_width_inc;
          units_h /= old_height_inc;

          /* update the 'invisible' geometry */
          wdiff = units_w * (Tmp_win->hints.width_inc - old_width_inc) +
            (Tmp_win->hints.base_width - old_base_width);
          hdiff = units_h * (Tmp_win->hints.height_inc - old_height_inc) +
            (Tmp_win->hints.base_height - old_base_height);
          gravity_resize(
            Tmp_win->hints.win_gravity, &Tmp_win->normal_g, wdiff, hdiff);
        }
        gravity_constrain_size(
          Tmp_win->hints.win_gravity, Tmp_win, &Tmp_win->normal_g);
        if (!IS_MAXIMIZED(Tmp_win))
        {
          rectangle new_g;

          get_relative_geometry(&new_g, &Tmp_win->normal_g);
          if (IS_SHADED(Tmp_win))
            get_shaded_geometry(Tmp_win, &new_g, &new_g);
          ForceSetupFrame(
            Tmp_win, new_g.x, new_g.y, new_g.width, new_g.height, False);
        }
        else
        {
	  int w;
	  int h;

          maximize_adjust_offset(Tmp_win);
	  /* domivogt (07-Apr-2000): as terrible hack to work around a xterm
	   * bug: when the font size is changed in a xterm, xterm simply assumes
	   * that the wm will grant its new size.  Of course this is wrong if
	   * the xterm is maximised.  To make xterm happy, we first send a
	   * ConfigureNotify with the current (maximised) geometry + 1 pixel in
	   * height, then another one with the correct old geometry.  Changing
	   * the font multiple times will cause the xterm to shrink because
	   * gravity_constrain_size doesn't know about the initially requested
	   * dimensions. */
	  w = Tmp_win->max_g.width;
	  h = Tmp_win->max_g.height;
	  gravity_constrain_size(
	    Tmp_win->hints.win_gravity, Tmp_win, &Tmp_win->max_g);
	  if (w != Tmp_win->max_g.width ||
	      h != Tmp_win->max_g.height)
	  {
	    rectangle new_g;

	    /* This is in case the size_inc changed and the old dimensions are
	     * not multiples of the new values. */
	    get_relative_geometry(&new_g, &Tmp_win->max_g);
	    if (IS_SHADED(Tmp_win))
	      get_shaded_geometry(Tmp_win, &new_g, &new_g);
	    ForceSetupFrame(
	      Tmp_win, new_g.x, new_g.y, new_g.width, new_g.height, False);
	  }
	  else
	  {
	    SendConfigureNotify(
	      Tmp_win, Tmp_win->frame_g.x, Tmp_win->frame_g.y,
	      Tmp_win->frame_g.width, Tmp_win->frame_g.height+1, 0, False);
	    XSync(dpy, 0);
	    /* free some CPU */
	    usleep(1);
	    SendConfigureNotify(
	      Tmp_win, Tmp_win->frame_g.x, Tmp_win->frame_g.y,
	      Tmp_win->frame_g.width, Tmp_win->frame_g.height, 0, False);
	    XSync(dpy, 0);
	  }
        }
        GNOME_SetWinArea(Tmp_win);
      }
      BroadcastConfig(M_CONFIGURE_WINDOW,Tmp_win);
      break;

    default:
      if(Event.xproperty.atom == _XA_WM_PROTOCOLS)
	FetchWmProtocols (Tmp_win);
      else if (Event.xproperty.atom == _XA_WM_COLORMAP_WINDOWS)
	{
	  FetchWmColormapWindows (Tmp_win);	/* frees old data */
	  ReInstallActiveColormap();
	}
      else if(Event.xproperty.atom == _XA_WM_STATE)
	{
	  if((Tmp_win != NULL)&&(HAS_CLICK_FOCUS(Tmp_win))
	     &&(Tmp_win == Scr.Focus))
	    {
              if (OnThisPage)
              {
	        Scr.Focus = NULL;
	        SetFocusWindow(Tmp_win, 0);
              }
	    }
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

  /* Process GNOME Messages */
  if (GNOME_ProcessClientMessage(Tmp_win, &Event))
  {
    return;
  }

  if ((Event.xclient.message_type == _XA_WM_CHANGE_STATE)&&
      (Tmp_win)&&(Event.xclient.data.l[0]==IconicState)&&
      !IS_ICONIFIED(Tmp_win))
  {
    XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
                   &(button.xmotion.x_root),
                   &(button.xmotion.y_root),
                   &JunkX, &JunkY, &JunkMask);
    button.type = 0;
    old_execute_function("Iconify", Tmp_win, &button, C_FRAME, -1, 0, NULL);
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
  if (Tmp_win)
  {
    if(Event.xclient.window != Tmp_win->w)
    {
      Event.xclient.window = Tmp_win->w;
      XSendEvent(dpy, Tmp_win->w, False, NoEventMask, &Event);
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
  if (Event.xexpose.count != 0)
    return;

  DBUG("HandleExpose","Routine Entered");

  if (Tmp_win)
  {
    draw_window_parts draw_parts;

    if (Event.xany.window == Tmp_win->title_w)
      draw_parts = DRAW_TITLE;
    else if (Event.xany.window == Tmp_win->decor_w ||
	     Event.xany.window == Tmp_win->frame)
      draw_parts = DRAW_FRAME;
    else
      draw_parts = DRAW_BUTTONS;
    DrawDecorations(
      Tmp_win, draw_parts, (Scr.Hilite == Tmp_win), True, Event.xany.window);
  }
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

  destroy_window(Tmp_win);
  GNOME_SetClientList();
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
  HandleMapRequestKeepRaised(None, NULL);
}
void HandleMapRequestKeepRaised(Window KeepRaised, FvwmWindow *ReuseWin)
{
  extern long isIconicState;
  extern Bool isIconifiedByParent;
  extern Boolean PPosOverride;
  Bool OnThisPage = False;

  Event.xany.window = Event.xmaprequest.window;

  if (ReuseWin == NULL)
  {
    if(XFindContext(dpy, Event.xany.window, FvwmContext,
		    (caddr_t *)&Tmp_win)==XCNOENT)
    {
      Tmp_win = NULL;
    }
  }
  else
  {
    Tmp_win = ReuseWin;
  }

  if(!PPosOverride)
    XFlush(dpy);

  /* If the window has never been mapped before ... */
  if(!Tmp_win || (Tmp_win && DO_REUSE_DESTROYED(Tmp_win)))
  {
    /* Add decorations. */
    Tmp_win = AddWindow(Event.xany.window, ReuseWin);
    if (Tmp_win == NULL)
      return;
  }
  /*
   * Make sure at least part of window is on this page
   * before giving it focus...
   */
  OnThisPage = IsRectangleOnThisPage(&(Tmp_win->frame_g), Tmp_win->Desk);

  if(KeepRaised != None)
    XRaiseWindow(dpy,KeepRaised);
  /* If it's not merely iconified, and we have hints, use them. */
  if (!IS_ICONIFIED(Tmp_win))
  {
    int state;

    if(Tmp_win->wmhints && (Tmp_win->wmhints->flags & StateHint))
      state = Tmp_win->wmhints->initial_state;
    else
      state = NormalState;

    if(DO_START_ICONIC(Tmp_win))
      state = IconicState;

    if(isIconicState != DontCareState)
      state = isIconicState;

    MyXGrabServer(dpy);
    switch (state)
    {
    case DontCareState:
    case NormalState:
    case InactiveState:
    default:
      if (Tmp_win->Desk == Scr.CurrentDesk)
      {
	Bool do_grab_focus;

	XMapWindow(dpy, Tmp_win->w);
	XMapWindow(dpy, Tmp_win->frame);
	SET_MAP_PENDING(Tmp_win, 1);
	SetMapStateProp(Tmp_win, NormalState);
	if (!OnThisPage)
	  do_grab_focus = True;
	else if (DO_GRAB_FOCUS(Tmp_win) &&
		 (!IS_TRANSIENT(Tmp_win) || Tmp_win->transientfor == Scr.Root))
	{
	  do_grab_focus = True;
	}
	else if (IS_TRANSIENT(Tmp_win) && DO_GRAB_FOCUS_TRANSIENT(Tmp_win) &&
		 Scr.Focus && Scr.Focus->w == Tmp_win->transientfor)
	{
	  /* it's a transient and its transientfor currently has focus. */
	  do_grab_focus = True;
	}
	else if (IS_TRANSIENT(Tmp_win) && DO_GRAB_FOCUS(Tmp_win) &&
		 !(XGetGeometry(dpy, Tmp_win->transientfor, &JunkRoot, &JunkX,
				&JunkY, &JunkWidth, &JunkHeight, &JunkBW,
				&JunkDepth)))
	{
	  /* Gee, the transientfor does not exist! These evil application
	   * programmers must hate us a lot. */
	  Tmp_win->transientfor = Scr.Root;
	  do_grab_focus = True;
	}
	else
	  do_grab_focus = False;
	if (do_grab_focus)
	{
	  SetFocusWindow(Tmp_win, 1);
	}
      }
      else
      {
	XMapWindow(dpy, Tmp_win->w);
	SetMapStateProp(Tmp_win, NormalState);
      }
      break;

    case IconicState:
      if (isIconifiedByParent)
      {
	isIconifiedByParent = False;
	SET_ICONIFIED_BY_PARENT(Tmp_win, 1);
      }
      if (Tmp_win->wmhints)
      {
	Iconify(Tmp_win, Tmp_win->wmhints->icon_x,
		Tmp_win->wmhints->icon_y);
      }
      else
      {
	Iconify(Tmp_win, 0, 0);
      }
      break;
    }
    if(!PPosOverride)
      XSync(dpy,0);
    MyXUngrabServer(dpy);
  }
  /* If no hints, or currently an icon, just "deiconify" */
  else
  {
    DeIconify(Tmp_win);
  }
  if (IS_SHADED(Tmp_win))
  {
    BroadcastPacket(M_WINDOWSHADE, 3, Tmp_win->w, Tmp_win->frame,
                    (unsigned long)Tmp_win);
  }

  if (!IS_ICONIFIED(Tmp_win) && Scr.Focus && Scr.Focus != Tmp_win &&
      !is_on_top_of_layer(Scr.Focus))
  {
    if (Tmp_win->Desk == Scr.CurrentDesk &&
	Tmp_win->frame_g.x + Tmp_win->frame_g.width > Scr.Focus->frame_g.x &&
	Scr.Focus->frame_g.x + Scr.Focus->frame_g.width > Tmp_win->frame_g.x &&
	Tmp_win->frame_g.y + Tmp_win->frame_g.height > Scr.Focus->frame_g.y &&
	Scr.Focus->frame_g.y + Scr.Focus->frame_g.height > Tmp_win->frame_g.y)
    {
      /* The newly mapped window overlaps the focused window. Make sure
       * ClickToFocusRaises and MouseFocusClickRaises work again.
       *
       * Note: There are many conditions under which we do not have to call
       * focus_grab_buttons(), but it is not worth the effort to write them
       * down here.  Rather do some unnecessary work in this function. */
      focus_grab_buttons(Scr.Focus, True);
    }
  }

  /* Just to be on the safe side, we make sure that STARTICONIC
     can only influence the initial transition from withdrawn state. */
  SET_DO_START_ICONIC(Tmp_win, 0);
  if (DO_DELETE_ICON_MOVED(Tmp_win))
  {
    SET_DELETE_ICON_MOVED(Tmp_win, 0);
    SET_ICON_MOVED(Tmp_win, 0);
  }
  /* Clean out the global so that it isn't used on additional map events. */
  isIconicState = DontCareState;
  GNOME_SetClientList();
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleMapNotify - MapNotify event handler
 *
 ***********************************************************************/
void HandleMapNotify(void)
{
  Bool OnThisPage = False;

  DBUG("HandleMapNotify","Routine Entered");

  if (!Tmp_win)
  {
    if((Event.xmap.override_redirect == True)&&
       (Event.xmap.window != Scr.NoFocusWin))
    {
      XSelectInput(dpy,Event.xmap.window,FocusChangeMask);
      Scr.UnknownWinFocused = Event.xmap.window;
    }
    return;
  }

  /* Except for identifying over-ride redirect window mappings, we
   * don't need or want windows associated with the substructurenotifymask */
  if(Event.xmap.event != Event.xmap.window)
    return;

  SET_MAP_PENDING(Tmp_win, 0);
  /* don't map if the event was caused by a de-iconify */
  if (IS_DEICONIFY_PENDING(Tmp_win))
  {
    return;
  }

  /*
      Make sure at least part of window is on this page
      before giving it focus...
  */
  OnThisPage = IsRectangleOnThisPage(&(Tmp_win->frame_g), Tmp_win->Desk);

  /*
   * Need to do the grab to avoid race condition of having server send
   * MapNotify to client before the frame gets mapped; this is bad because
   * the client would think that the window has a chance of being viewable
   * when it really isn't.
   */
  MyXGrabServer (dpy);
  if (Tmp_win->icon_w)
    XUnmapWindow(dpy, Tmp_win->icon_w);
  if(Tmp_win->icon_pixmap_w != None)
    XUnmapWindow(dpy, Tmp_win->icon_pixmap_w);
  XMapSubwindows(dpy, Tmp_win->frame);
  XMapSubwindows(dpy, Tmp_win->decor_w);

  if(Tmp_win->Desk == Scr.CurrentDesk)
  {
    XMapWindow(dpy, Tmp_win->frame);
  }

  if(IS_ICONIFIED(Tmp_win))
    BroadcastPacket(M_DEICONIFY, 3,
                    Tmp_win->w, Tmp_win->frame, (unsigned long)Tmp_win);
  else
    BroadcastPacket(M_MAP, 3,
                    Tmp_win->w,Tmp_win->frame, (unsigned long)Tmp_win);

  if((!IS_TRANSIENT(Tmp_win) && DO_GRAB_FOCUS(Tmp_win)) ||
     (IS_TRANSIENT(Tmp_win) && DO_GRAB_FOCUS_TRANSIENT(Tmp_win) &&
      Scr.Focus && Scr.Focus->w == Tmp_win->transientfor))
  {
    if (OnThisPage)
    {
      SetFocusWindow(Tmp_win, 1);
    }
  }
  if((!(HAS_BORDER(Tmp_win)|HAS_TITLE(Tmp_win)))&&(Tmp_win->boundary_width <2))
  {
    DrawDecorations(Tmp_win, DRAW_ALL, False, True, Tmp_win->decor_w);
  }
  XSync(dpy,0);
  MyXUngrabServer (dpy);
  XFlush (dpy);
  SET_MAPPED(Tmp_win, 1);
  SET_ICONIFIED(Tmp_win, 0);
  SET_ICON_UNMAPPED(Tmp_win, 0);
  if (DO_ICONIFY_AFTER_MAP(Tmp_win))
  {
    /* finally, if iconification was requested before the window was mapped,
     * request it now. */
    Iconify(Tmp_win, 0, 0);
    SET_ICONIFY_AFTER_MAP(Tmp_win, 0);
  }
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
  extern FvwmWindow *colormap_win;
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
  if (!Tmp_win)
    {
      Event.xany.window = Event.xunmap.window;
      weMustUnmap = 1;
      if (XFindContext(dpy, Event.xany.window,
		       FvwmContext, (caddr_t *)&Tmp_win) == XCNOENT)
	Tmp_win = NULL;
    }

  if(!Tmp_win)
    return;

  if (Event.xunmap.window == Tmp_win->frame)
  {
    SET_DEICONIFY_PENDING(Tmp_win , 0);
  }

  if (must_return)
    return;

  if(weMustUnmap)
    XUnmapWindow(dpy, Event.xunmap.window);

  if(Tmp_win ==  Scr.Hilite)
    Scr.Hilite = NULL;

  if(Scr.PreviousFocus == Tmp_win)
    Scr.PreviousFocus = NULL;

  focus_grabbed = (Tmp_win == Scr.Focus) &&
    ((!IS_TRANSIENT(Tmp_win) && DO_GRAB_FOCUS(Tmp_win)) ||
     (IS_TRANSIENT(Tmp_win) && DO_GRAB_FOCUS_TRANSIENT(Tmp_win)));

  if((Tmp_win == Scr.Focus)&&(HAS_CLICK_FOCUS(Tmp_win)))
  {
    if(Tmp_win->next)
      SetFocusWindow(Tmp_win->next, 1);
    else
      DeleteFocus(1);
  }

  if(Scr.Focus == Tmp_win)
    DeleteFocus(1);

  if(Tmp_win == Scr.pushed_window)
    Scr.pushed_window = NULL;

  if(Tmp_win == colormap_win)
    colormap_win = NULL;

  if (!IS_MAPPED(Tmp_win) && !IS_ICONIFIED(Tmp_win))
  {
    return;
  }

  MyXGrabServer(dpy);

  if(XCheckTypedWindowEvent (dpy, Event.xunmap.window, DestroyNotify,&dummy))
  {
    destroy_window(Tmp_win);
  }
  else
  /*
   * The program may have unmapped the client window, from either
   * NormalState or IconicState.  Handle the transition to WithdrawnState.
   *
   * We need to reparent the window back to the root (so that fvwm exiting
   * won't cause it to get mapped) and then throw away all state (pretend
   * that we've received a DestroyNotify).
   */
  if (XTranslateCoordinates (dpy, Event.xunmap.window, Scr.Root,
			     0, 0, &dstx, &dsty, &dumwin))
  {
    XEvent ev;
    Bool reparented;

    reparented = XCheckTypedWindowEvent (dpy, Event.xunmap.window,
					 ReparentNotify, &ev);
    SetMapStateProp (Tmp_win, WithdrawnState);
    if (reparented)
    {
      if (Tmp_win->old_bw)
	XSetWindowBorderWidth (dpy, Event.xunmap.window, Tmp_win->old_bw);
      if((!IS_ICON_SUPPRESSED(Tmp_win))&&
	 (Tmp_win->wmhints && (Tmp_win->wmhints->flags & IconWindowHint)))
	XUnmapWindow (dpy, Tmp_win->wmhints->icon_window);
    }
    else
    {
      RestoreWithdrawnLocation (Tmp_win,False);
    }
    XRemoveFromSaveSet (dpy, Event.xunmap.window);
    XSelectInput (dpy, Event.xunmap.window, NoEventMask);
    destroy_window(Tmp_win);		/* do not need to mash event before */
    /*
     * Flush any pending events for the window.
     */
    /* Bzzt! it could be about to re-map */
/*      while(XCheckWindowEvent(dpy, Event.xunmap.window,
			      StructureNotifyMask | PropertyChangeMask |
			      ColormapChangeMask | VisibilityChangeMask |
			      EnterWindowMask | LeaveWindowMask, &dummy));
*/
  } /* else window no longer exists and we'll get a destroy notify */
  MyXUngrabServer(dpy);

  XFlush (dpy);

  if (focus_grabbed)
  {
    CoerceEnterNotifyOnCurrentWindow();
  }
  GNOME_SetClientList();
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

  DBUG("HandleButtonPress","Routine Entered");

  if (!Tmp_win && Event.xany.window != Scr.Root)
  {
    /* event in unmanaged window or subwindow of a client */
    XSync(dpy,0);
    XAllowEvents(dpy,ReplayPointer,CurrentTime);
    XSync(dpy,0);
    return;
  }
  if (Event.xbutton.subwindow != None &&
      (Tmp_win == None || Event.xany.window != Tmp_win->w))
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
    return;
  }
  if (Tmp_win && HAS_NEVER_FOCUS(Tmp_win))
  {
    /* It might seem odd to try to focus a window that never is given focus by
     * fvwm, but the window might want to take focus itself, and SetFocus will
     * tell it to do so in this case instead of giving it focus. */
    SetFocusWindow(Tmp_win, 1);
  }
  /* click to focus stuff goes here */
  if((Tmp_win)&&(HAS_CLICK_FOCUS(Tmp_win))&&(Tmp_win != Scr.Ungrabbed))
  {
    SetFocusWindow(Tmp_win, 1);
    /* RBW - 12/09/.1999- I'm not sure we need to check both cases, but
       I'll leave this as is for now.  */
    if (!DO_NOT_RAISE_CLICK_FOCUS_CLICK(Tmp_win)
#if 0
	/* DV - this forces that every focus click on the decorations raises
	 * the window.  This somewhat negates the ClickToFocusRaisesOff style.
	 */
	||
	((Event.xany.window != Tmp_win->w)&&
	 (Event.xbutton.subwindow != Tmp_win->w)&&
	 (Event.xany.window != Tmp_win->Parent)&&
	 (Event.xbutton.subwindow != Tmp_win->Parent))
#endif
      )
    {
      /* We can't raise the window immediately because the action bound to the
       * click might be "Lower" or "RaiseLower". So mark the window as scheduled
       * to be raised after the binding is executed. Functions that modify the
       * stacking order will reset this flag. */
      SET_SCHEDULED_FOR_RAISE(Tmp_win, 1);
    }

    Context = GetContext(Tmp_win,&Event, &PressedW);
    if (!IS_ICONIFIED(Tmp_win) && Context == C_WINDOW)
    {
      if (Tmp_win && IS_SCHEDULED_FOR_RAISE(Tmp_win))
      {
	RaiseWindow(Tmp_win);
	SET_SCHEDULED_FOR_RAISE(Tmp_win, 0);
      }
      XSync(dpy,0);
      /* pass click event to just clicked to focus window? Do not swallow the
       * click if the window didn't accept the focus */
      if (!DO_NOT_PASS_CLICK_FOCUS_CLICK(Tmp_win) || Scr.Focus != Tmp_win)
      {
	XAllowEvents(dpy,ReplayPointer,CurrentTime);
      }
      else /* don't pass click to just focused window */
      {
	XAllowEvents(dpy,AsyncPointer,CurrentTime);
      }
      XSync(dpy,0);
      return;
    }
    if (!IS_ICONIFIED(Tmp_win))
    {
      DrawDecorations(Tmp_win, DRAW_ALL, True, True, PressedW);
    }
  }
  else if (Tmp_win && Event.xbutton.window == Tmp_win->Parent &&
	   (HAS_SLOPPY_FOCUS(Tmp_win) || HAS_MOUSE_FOCUS(Tmp_win) ||
	    HAS_NEVER_FOCUS(Tmp_win)) &&
	   DO_RAISE_MOUSE_FOCUS_CLICK(Tmp_win))
  {
    FvwmWindow *tmp = Scr.Ungrabbed;

    /*
      RBW - Release the Parent grab here (whether we raise or not). We
      have to wait till this point or we would miss the raise click, which
      is not contemporaneous with the focus change.
      Scr.Ungrabbed should always be NULL here. I don't know anything
      useful we could do if it's not, other than ignore this window.
    */
    if (!is_on_top_of_layer(Tmp_win) &&
        MaskUsedModifiers(Event.xbutton.state) == 0)
    {
      RaiseWindow(Tmp_win);
      focus_grab_buttons(Tmp_win, True);
      Scr.Ungrabbed = tmp;
      XSync(dpy,0);
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
      XSync(dpy,0);
      return;
    }
    focus_grab_buttons(Tmp_win, True);
    Scr.Ungrabbed = tmp;
  }

  XSync(dpy,0);
  XAllowEvents(dpy,ReplayPointer,CurrentTime);
  XSync(dpy,0);

  Context = GetContext(Tmp_win, &Event, &PressedW);
  LocalContext = Context;
  if (Tmp_win)
  {
    if (Context == C_TITLE)
      DrawDecorations(
        Tmp_win, DRAW_TITLE, (Scr.Hilite == Tmp_win), True, None);
    else if (Context & (C_LALL | C_RALL))
      DrawDecorations(
        Tmp_win, DRAW_BUTTONS, (Scr.Hilite == Tmp_win), True, PressedW);
    else
    {
      DrawDecorations(
        Tmp_win, DRAW_FRAME, (Scr.Hilite == Tmp_win),
        (HAS_DEPRESSABLE_BORDER(Tmp_win) && PressedW != None), PressedW);
    }
  }

  ButtonWindow = Tmp_win;

  /* we have to execute a function or pop up a menu */
  STROKE_CODE(stroke_init());
  STROKE_CODE(send_motion = TRUE);
  /* need to search for an appropriate mouse binding */
  action = CheckBinding(Scr.AllBindings, STROKE_ARG(0) Event.xbutton.button,
			Event.xbutton.state, GetUnusedModifiers(), Context,
			MOUSE_BINDING);
  if (action != NULL && (action[0] != 0))
  {
    old_execute_function(
      action, Tmp_win, &Event, Context, -1, FUNC_DO_SYNC_BUTTONS, NULL);
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

  if (ButtonWindow && IS_SCHEDULED_FOR_RAISE(ButtonWindow))
  {
    /* now that we know the action did not restack the window we can raise it.
     */
    RaiseWindow(ButtonWindow);
    SET_SCHEDULED_FOR_RAISE(ButtonWindow, 0);
  }

  OldPressedW = PressedW;
  PressedW = None;
  if (ButtonWindow && check_if_fvwm_window_exists(ButtonWindow))
  {
    if (LocalContext == C_TITLE)
      DrawDecorations(
        ButtonWindow, DRAW_TITLE, (Scr.Hilite == ButtonWindow), True, None);
    else if (LocalContext & (C_LALL | C_RALL))
      DrawDecorations(
        ButtonWindow, DRAW_BUTTONS, (Scr.Hilite == ButtonWindow), True,
        OldPressedW);
    else
      DrawDecorations(
        ButtonWindow, DRAW_FRAME, (Scr.Hilite == ButtonWindow),
        HAS_DEPRESSABLE_BORDER(ButtonWindow), None);
  }
  ButtonWindow = NULL;
  /* Release any automatic or passive grabs */
  XUngrabPointer(dpy, CurrentTime);
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

   Context = GetContext(Tmp_win,&Event, &dummy);

   /*  Allows modifier to work (Only R context works here). */
   real_modifier = Event.xbutton.state - (1 << (7 + Event.xbutton.button));

   /* need to search for an appropriate stroke binding */
   action = CheckBinding(
     Scr.AllBindings, sequence, Event.xbutton.button, real_modifier,
     GetUnusedModifiers(), Context, STROKE_BINDING);
   /* got a match, now process it */
   if (action != NULL && (action[0] != 0))
   {
     old_execute_function(
       action, Tmp_win, &Event, Context, -1, FUNC_DO_SYNC_BUTTONS, NULL);
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

  DBUG("HandleEnterNotify","Routine Entered");

  /* Ignore EnterNotify events while a window is resized or moved as a wire
   * frame; otherwise the window list may be screwed up. */
  if (Scr.flags.is_wire_frame_displayed)
    return;

  /* look for a matching leaveNotify which would nullify this enterNotify */
  if(XCheckTypedWindowEvent (dpy, ewp->window, LeaveNotify, &d))
  {
    /*
     * RBW - if we're in startup, this is a coerced focus, so we don't
     * want to save the event time, or exit prematurely.
     */
    if (!fFvwmInStartup)
    {
      StashEventTime(&d);
      if((d.xcrossing.mode==NotifyNormal)&&
	 (d.xcrossing.detail!=NotifyInferior))
	return;
    }
  }

  /* an EnterEvent in one of the PanFrameWindows activates the Paging */
  if (ewp->window==Scr.PanFrameTop.win
      || ewp->window==Scr.PanFrameLeft.win
      || ewp->window==Scr.PanFrameRight.win
      || ewp->window==Scr.PanFrameBottom.win )
  {
    int delta_x=0, delta_y=0;
    /* this was in the HandleMotionNotify before, HEDU */
    HandlePaging(Scr.EdgeScrollX,Scr.EdgeScrollY,
                 &ewp->x_root,&ewp->y_root,
                 &delta_x,&delta_y,True,True,False);
    return;
  }

  if (ewp->window == Scr.Root)
  {
    if (!Scr.flags.is_pointer_on_this_screen)
    {
      Scr.flags.is_pointer_on_this_screen = 1;
      if (Scr.LastScreenFocus &&
	  (HAS_SLOPPY_FOCUS(Scr.LastScreenFocus) ||
	   HAS_CLICK_FOCUS(Scr.LastScreenFocus)))
      {
	SetFocusWindow(Scr.LastScreenFocus, 1);
      }
      else
      {
	ForceDeleteFocus(1);
      }
      Scr.LastScreenFocus = NULL;
    }
    else if (!Scr.Focus || HAS_MOUSE_FOCUS(Scr.Focus))
    {
      DeleteFocus(1);
    }
    if (Scr.ColormapFocus == COLORMAP_FOLLOWS_MOUSE)
    {
      InstallWindowColormaps(NULL);
    }
    return;
  }

  /* make sure its for one of our windows */
  if (!Tmp_win)
  {
    /* handle a subwindow cmap */
    EnterSubWindowColormap(Event.xany.window);
    return;
  }

  if (HAS_MOUSE_FOCUS(Tmp_win) || HAS_SLOPPY_FOCUS(Tmp_win))
  {
    SetFocusWindow(Tmp_win, 1);
  }
  else if (HAS_NEVER_FOCUS(Tmp_win))
  {
    /* Give the window a chance to grab the buttons needed for raise-on-click */
    if (Scr.Focus != Tmp_win)
    {
      focus_grab_buttons(Tmp_win, False);
      focus_grab_buttons(Scr.Focus, True);
    }
  }
  else if (HAS_CLICK_FOCUS(Tmp_win) && Tmp_win == Scr.Focus &&
	   do_accept_input_focus(Tmp_win))
  {
    /* We have to refresh the focus window here in case we left the focused
     * fvwm window.  Motif apps may lose the input focus otherwise.  But do not
     * try to refresh the focus of applications that want to handle it
     * themselves. */
    FOCUS_SET(Tmp_win->w);
  }
  if (Scr.ColormapFocus == COLORMAP_FOLLOWS_MOUSE)
  {
    if((!IS_ICONIFIED(Tmp_win))&&(Event.xany.window == Tmp_win->w))
      InstallWindowColormaps(Tmp_win);
    else
      InstallWindowColormaps(NULL);
  }

  /* We get an EnterNotify with mode == UnGrab when fvwm releases
     the grab held during iconification. We have to ignore this,
     or icon title will be initially raised. */
  if (IS_ICONIFIED(Tmp_win) && (ewp->mode == NotifyNormal))
  {
    SET_ICON_ENTERED(Tmp_win,1);
    DrawIconWindow(Tmp_win);
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
    return;

  /* CDE-like behaviour of raising the icon title if the icon
     gets the focus (in particular if the cursor is over the icon) */
  if (Tmp_win && IS_ICONIFIED(Tmp_win))
  {
    SET_ICON_ENTERED(Tmp_win,0);
    DrawIconWindow (Tmp_win);
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
	Scr.flags.is_pointer_on_this_screen = 0;
	Scr.LastScreenFocus = Scr.Focus;
	if (Scr.Focus != NULL)
	  DeleteFocus(1);
	if (Scr.Hilite != NULL)
	  DrawDecorations(Scr.Hilite, DRAW_ALL, False, True, None);
      }
    }
  }
  else
  {
    /* handle a subwindow cmap */
    LeaveSubWindowColormap(Event.xany.window);
  }
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
   * Event.xany.window is Event.xconfigurerequest.parent, so Tmp_win will
   * be wrong
   */
  Event.xany.window = cre->window;	/* mash parent field */
  if (XFindContext (dpy, cre->window, FvwmContext, (caddr_t *) &Tmp_win) ==
      XCNOENT)
    Tmp_win = NULL;

#define EXPERIMENTAL_ANTI_RACE_CONDITION_CODE
#ifdef EXPERIMENTAL_ANTI_RACE_CONDITION_CODE
  /* merge all pending ConfigureRequests for the window into a single event */
  if (Tmp_win)
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
  if (!Tmp_win || cre->window == Tmp_win->icon_w ||
      cre->window == Tmp_win->icon_pixmap_w)
  {
    xwcm = cre->value_mask &
      (CWX | CWY | CWWidth | CWHeight | CWBorderWidth);
    xwc.x = cre->x;
    xwc.y = cre->y;
    if((Tmp_win)&&((Tmp_win->icon_pixmap_w == cre->window)))
    {
      Tmp_win->icon_p_height = cre->height+ cre->border_width +
	cre->border_width;
    }
    else if((Tmp_win)&&((Tmp_win->icon_w == cre->window)))
    {
      Tmp_win->icon_xl_loc = cre->x;
      Tmp_win->icon_g.x = cre->x +
        (Tmp_win->icon_g.width - Tmp_win->icon_p_width)/2;
      Tmp_win->icon_g.y = cre->y - Tmp_win->icon_p_height;
      if(!IS_ICON_UNMAPPED(Tmp_win))
        BroadcastPacket(M_ICON_LOCATION, 7,
                        Tmp_win->w, Tmp_win->frame,
                        (unsigned long)Tmp_win,
                        Tmp_win->icon_g.x, Tmp_win->icon_g.y,
                        Tmp_win->icon_p_width,
                        Tmp_win->icon_g.height + Tmp_win->icon_p_height);
    }
    xwc.width = cre->width;
    xwc.height = cre->height;
    xwc.border_width = cre->border_width;

    XConfigureWindow(dpy, Event.xany.window, xwcm, &xwc);

    if(Tmp_win)
    {
      if (cre->window != Tmp_win->icon_pixmap_w &&
	  Tmp_win->icon_pixmap_w != None)
      {
	xwc.x = Tmp_win->icon_g.x;
	xwc.y = Tmp_win->icon_g.y - Tmp_win->icon_p_height;
	xwcm = cre->value_mask & (CWX | CWY);
	XConfigureWindow(dpy, Tmp_win->icon_pixmap_w, xwcm, &xwc);
      }
      if(Tmp_win->icon_w != None)
      {
	xwc.x = Tmp_win->icon_g.x;
	xwc.y = Tmp_win->icon_g.y;
	xwcm = cre->value_mask & (CWX | CWY);
        XConfigureWindow(dpy, Tmp_win->icon_w, xwcm, &xwc);
      }
    }
    if (!Tmp_win)
      return;
  }

#ifdef SHAPE
  if (ShapesSupported)
  {
    int xws, yws, xbs, ybs;
    unsigned wws, hws, wbs, hbs;
    int boundingShaped, clipShaped;

    if (XShapeQueryExtents(dpy, Tmp_win->w,&boundingShaped, &xws, &yws, &wws,
			   &hws,&clipShaped, &xbs, &ybs, &wbs, &hbs))
    {
      Tmp_win->wShaped = boundingShaped;
    }
    else
    {
      Tmp_win->wShaped = 0;
    }
  }
#endif /* SHAPE */

  if (cre->window == Tmp_win->w)
  {
#if 0
fprintf(stderr, "cre: %d(%d) %d(%d) %d(%d)x%d(%d)\n",
        cre->x, (int)(cre->value_mask & CWX),
        cre->y, (int)(cre->value_mask & CWY),
        cre->width, (int)(cre->value_mask & CWWidth),
        cre->height, (int)(cre->value_mask & CWHeight));
#endif
    /* Don't modify frame_XXX fields before calling SetupWindow! */
    dx = 0;
    dy = 0;
    dw = 0;
    dh = 0;

    /* for restoring */
    if (cre->value_mask & CWBorderWidth)
    {
      Tmp_win->old_bw = cre->border_width;
    }
    /* override even if border change */

    if (cre->value_mask & CWX)
      dx = cre->x - Tmp_win->frame_g.x - Tmp_win->boundary_width;
    if (cre->value_mask & CWY)
      dy = cre->y - Tmp_win->frame_g.y - Tmp_win->boundary_width -
	Tmp_win->title_g.height;
    if (cre->value_mask & CWWidth)
      dw = cre->width - (Tmp_win->frame_g.width - 2 * Tmp_win->boundary_width);

    if (cre->value_mask & CWHeight)
    {
      if (cre->height < (WINDOW_FREAKED_OUT_HEIGHT - Tmp_win->title_g.height -
			 2 * Tmp_win->boundary_width))
      {
	dh = cre->height - (Tmp_win->frame_g.height -
			    2 * Tmp_win->boundary_width -
			    Tmp_win->title_g.height);
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

    /*
     * SetupWindow (x,y) are the location of the upper-left outer corner and
     * are passed directly to XMoveResizeWindow (frame).  The (width,height)
     * are the inner size of the frame.  The inner width is the same as the
     * requested client window width; the inner height is the same as the
     * requested client window height plus any title bar slop.
     */
    new_g = Tmp_win->frame_g;
    if (IS_SHADED(Tmp_win))
      new_g.height = Tmp_win->normal_g.height;
    oldnew_w = new_g.width + dw;
    oldnew_h = new_g.height + dh;
    constr_w = oldnew_w;
    constr_h = oldnew_h;
    constrain_size(
      Tmp_win, (unsigned int *)&constr_w, (unsigned int *)&constr_h, 0, 0,
      False);
    dw += (constr_w - oldnew_w);
    dh += (constr_h - oldnew_h);
    if (dx && dw)
    {
      new_g.x = Tmp_win->frame_g.x + dx;
      new_g.width = Tmp_win->frame_g.width + dw;
    }
    else if (dx && !dw)
    {
      new_g.x = Tmp_win->frame_g.x + dx;
    }
    else if (!dx && dw)
    {
      gravity_resize(Tmp_win->hints.win_gravity, &new_g, dw, 0);
    }
    if (dy && dh)
    {
      new_g.y = Tmp_win->frame_g.y + dy;
      new_g.height = Tmp_win->frame_g.height + dh;
    }
    else if (dy && !dh)
    {
      new_g.y = Tmp_win->frame_g.y + dy;
    }
    else if (!dy && dh)
    {
      gravity_resize(Tmp_win->hints.win_gravity, &new_g, 0, dh);
    }

    /* dont allow clients to resize maximized windows */
    if (!IS_MAXIMIZED(Tmp_win) || (!dw && !dh))
    {
      if (IS_SHADED(Tmp_win))
	get_shaded_geometry(Tmp_win, &new_g, &new_g);
      SetupFrame(
	Tmp_win, new_g.x, new_g.y, new_g.width, new_g.height, False);
    }
    /* make sure the window structure has the new position */
    update_absolute_geometry(Tmp_win);
    maximize_adjust_offset(Tmp_win);
    GNOME_SetWinArea(Tmp_win);
  }

  /*  Stacking order change requested...  */
  /*  Handle this *after* geometry changes, since we need the new
      geometry in occlusion calculations */
  if ( (cre->value_mask & CWStackMode) && !DO_IGNORE_RESTACK(Tmp_win) )
    {
      FvwmWindow *otherwin = NULL;

      if (cre->value_mask & CWSibling)
      {
        if (XFindContext (dpy, cre->above, FvwmContext,
			  (caddr_t *) &otherwin) == XCNOENT)
	{
          otherwin = NULL;
	}
      }

      if ((cre->detail != Above) && (cre->detail != Below))
	{
	   HandleUnusualStackmodes (cre->detail, Tmp_win, cre->window,
                                                 otherwin, cre->above);
	}
      /* only allow clients to restack windows within their layer */
      else if (!otherwin || compare_window_layers(otherwin, Tmp_win) != 0)
	{
	  switch (cre->detail)
	    {
            case Above:
              RaiseWindow (Tmp_win);
              break;
            case Below:
              LowerWindow (Tmp_win);
              break;
	    }
	}
      else
	{
	  xwc.sibling = otherwin->frame;
	  xwc.stack_mode = cre->detail;
	  xwcm = CWSibling | CWStackMode;
	  XConfigureWindow (dpy, Tmp_win->frame, xwcm, &xwc);

	  /* Maintain the condition that icon windows are stacked
	     immediately below their frame */
	  /* 1. for Tmp_win */
	  xwc.sibling = Tmp_win->frame;
	  xwc.stack_mode = Below;
	  xwcm = CWSibling | CWStackMode;
	  if (Tmp_win->icon_w != None)
	    {
	      XConfigureWindow(dpy, Tmp_win->icon_w, xwcm, &xwc);
	    }
	  if (Tmp_win->icon_pixmap_w != None)
	    {
	      XConfigureWindow(dpy, Tmp_win->icon_pixmap_w, xwcm, &xwc);
	    }

	  /* 2. for otherwin */
	  if (cre->detail == Below)
	    {
	      xwc.sibling = otherwin->frame;
	      xwc.stack_mode = Below;
	      xwcm = CWSibling | CWStackMode;
	      if (otherwin->icon_w != None)
		{
		  XConfigureWindow(dpy, otherwin->icon_w, xwcm, &xwc);
		}
	      if (otherwin->icon_pixmap_w != None)
		{
		  XConfigureWindow(dpy, otherwin->icon_pixmap_w, xwcm, &xwc);
		}
	    }

	  /* Maintain the stacking order ring */
	  if (cre->detail == Above)
	    {
	      remove_window_from_stack_ring(Tmp_win);
	      add_window_to_stack_ring_after(
		Tmp_win, get_prev_window_in_stack_ring(otherwin));
	    }
	  else /* cre->detail == Below */
	    {
	      remove_window_from_stack_ring(Tmp_win);
	      add_window_to_stack_ring_after(Tmp_win, otherwin);
	    }

	  /*
	    Let the modules know that Tmp_win changed its place
	    in the stacking order
	  */
	  BroadcastRestackThisWindow(Tmp_win);
	}
    }

#if 1
  /* This causes some ddd windows not to be drawn properly. Reverted back to
   * the old method in SetupFrame. */
  /* domivogt (15-Oct-1999): enabled this to work around buggy apps that
   * ask for a nonsense height and expect that they really get it. */
  if (do_send_event)
    {
      SendConfigureNotify(
	Tmp_win, Tmp_win->frame_g.x, Tmp_win->frame_g.y,
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
  if (!tmp_win || IS_SHADED(tmp_win))
    return;
  {
    XEvent client_event;

    client_event.type = ConfigureNotify;
    client_event.xconfigure.display = dpy;
    client_event.xconfigure.event = tmp_win->w;
    client_event.xconfigure.window = tmp_win->w;
    client_event.xconfigure.x = x + tmp_win->boundary_width;
    client_event.xconfigure.y = y + tmp_win->boundary_width +
      ((HAS_BOTTOM_TITLE(tmp_win)) ? 0 : tmp_win->title_g.height);
    client_event.xconfigure.width = w - 2 * tmp_win->boundary_width;
    client_event.xconfigure.height = h -
      2 * tmp_win->boundary_width - tmp_win->title_g.height;
    client_event.xconfigure.border_width = bw;
    client_event.xconfigure.above = tmp_win->frame;
    client_event.xconfigure.override_redirect = False;
    XSendEvent(dpy, tmp_win->w, False, StructureNotifyMask, &client_event);
    if (send_for_frame_too)
    {
      /* This is for buggy tk, which waits for the real ConfigureNotify
       * on frame instead of the synthetic one on w. The geometry data
       * in the event will not be correct for the frame, but tk doesn't
       * look at that data anyway. */
      client_event.xconfigure.event = tmp_win->frame;
      client_event.xconfigure.window = tmp_win->frame;
      XSendEvent(dpy, tmp_win->frame, False,StructureNotifyMask,&client_event);
    }
  }
}

/***********************************************************************
 *
 *  Procedure:
 *      HandleShapeNotify - shape notification event handler
 *
 ***********************************************************************/
#ifdef SHAPE
void HandleShapeNotify (void)
{
  DBUG("HandleShapeNotify","Routine Entered");

  if (ShapesSupported)
  {
    XShapeEvent *sev = (XShapeEvent *) &Event;

    if (!Tmp_win)
      return;
    if (sev->kind != ShapeBounding)
      return;
    Tmp_win->wShaped = sev->shaped;
    SetShape(Tmp_win,Tmp_win->frame_g.width);
  }
}
#endif  /* SHAPE*/

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

  if(Tmp_win && Tmp_win->frame == last_event_window)
    {
      if(vevent->state == VisibilityUnobscured)
      {
	SET_FULLY_VISIBLE(Tmp_win, 1);
	SET_PARTIALLY_VISIBLE(Tmp_win, 1);
      }
      else if (vevent->state == VisibilityPartiallyObscured)
      {
	SET_FULLY_VISIBLE(Tmp_win, 0);
	SET_PARTIALLY_VISIBLE(Tmp_win, 1);
      }
      else
      {
	SET_FULLY_VISIBLE(Tmp_win, 0);
	SET_PARTIALLY_VISIBLE(Tmp_win, 0);
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
    /* The timeouts become undefined whenever the select returns, and so
     * we have to reinitialise them */
    timeout.tv_sec = 42;
    timeout.tv_usec = 0;

    FD_ZERO(&in_fdset);
    FD_ZERO(&out_fdset);
    FD_SET(x_fd, &in_fdset);

    /* nothing is done here if fvwm was compiled without session support */
    if (sm_fd >= 0)
      FD_SET(sm_fd, &in_fdset);

    for(i=0; i<npipes; i++) {
      if(readPipes[i]>=0)
        FD_SET(readPipes[i], &in_fdset);
      if(pipeQueue[i]!= NULL)
        FD_SET(writePipes[i], &out_fdset);
    }

    DBUG("My_XNextEvent","waiting for module input/output");
    num_fd = fvwmSelect(fd_width, &in_fdset, &out_fdset, 0, timeoutP);

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
#ifdef SHAPE
  if (ShapesSupported)
    EventHandlerJumpTable[ShapeEventBase+ShapeNotify] = HandleShapeNotify;
#endif /* SHAPE */
  EventHandlerJumpTable[SelectionClear]   = HandleSelectionClear;
  EventHandlerJumpTable[SelectionRequest] = HandleSelectionRequest;
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
void DispatchEvent(Bool preserve_Tmp_win)
{
  Window w = Event.xany.window;
  FvwmWindow *s_Tmp_win = NULL;

  DBUG("DispatchEvent","Routine Entered");

  if (preserve_Tmp_win)
    s_Tmp_win = Tmp_win;
  StashEventTime(&Event);

  XFlush(dpy);
  if (XFindContext (dpy, w, FvwmContext, (caddr_t *) &Tmp_win) == XCNOENT)
  {
    Tmp_win = NULL;
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

  if (preserve_Tmp_win)
    Tmp_win = s_Tmp_win;
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
    Tmp_win = t;
  }
  if (e->type == ButtonPress && t && e->xkey.window == t->frame &&
      e->xkey.subwindow != None)
  {
    /* Translate frame coordinates into subwindow coordinates. */
    e->xkey.window = e->xkey.subwindow;
    XTranslateCoordinates(
      dpy, t->frame, e->xkey.subwindow, e->xkey.x, e->xkey.y, &(e->xkey.x),
      &(e->xkey.y), &(e->xkey.subwindow));
    if (e->xkey.window == t->Parent)
    {
      e->xkey.window = e->xkey.subwindow;
      XTranslateCoordinates(
	dpy, t->Parent, e->xkey.subwindow, e->xkey.x, e->xkey.y, &(e->xkey.x),
	&(e->xkey.y), &(e->xkey.subwindow));
    }
  }
  if(!t)
    return C_ROOT;

  if (e->type == KeyPress && e->xkey.window == t->frame &&
      e->xkey.subwindow == t->decor_w)
  {
    /* We can't get keyboard events on the decor_w directly beacause it is a
     * sibling of the parent window which gets all keyboard input. So we have to
     * grab keys on the frame and then translate the coordinates to find out in
     * which subwindow of the decor_w the event occured. */
    e->xkey.window = e->xkey.subwindow;
    XTranslateCoordinates(dpy, t->frame, t->decor_w, e->xkey.x, e->xkey.y,
			  &JunkX, &JunkY, &(e->xkey.subwindow));
  }
  *w= e->xany.window;

  if (*w == Scr.NoFocusWin)
    return C_ROOT;
  if (e->type == KeyPress && e->xkey.window == t->frame &&
      e->xkey.subwindow == t->decor_w)
  {
    /* We can't get keyboard events on the decor_w directly beacause it is a
     * sibling of the parent window which gets all keyboard input. So we have to
     * grab keys on the frame and then translate the coordinates to find out in
     * which subwindow of the decor_w the event occured. */
    e->xkey.window = e->xkey.subwindow;
    XTranslateCoordinates(dpy, t->frame, t->decor_w, e->xkey.x, e->xkey.y,
			  &JunkX, &JunkY, &(e->xkey.subwindow));
  }
  *w= e->xany.window;

  if (*w == Scr.NoFocusWin)
    return C_ROOT;
  if (e->xkey.subwindow != None && e->xany.window != t->w)
    *w = e->xkey.subwindow;
  if (*w == Scr.Root)
    return C_ROOT;
  if (t)
    {
      if (*w == t->title_w)
	Context = C_TITLE;
      else if (*w == t->w || *w == t->Parent || *w == t->frame)
	Context = C_WINDOW;
      else if (*w == t->icon_w || *w == t->icon_pixmap_w)
	Context = C_ICON;
      else if (*w == t->decor_w)
	Context = C_SIDEBAR;
      else
      {
	for(i=0;i<4;i++)
	  {
	    if(*w == t->corners[i])
	      {
		Context = C_FRAME;
		break;
	      }
	    if(*w == t->sides[i])
	      {
		Context = C_SIDEBAR;
		break;
	      }
	  }
	if (i < 4)
	  Button = i;
	else
	  {
	    for (i = 0; i < NUMBER_OF_BUTTONS; i++)
	    {
	      if (*w == t->button_w[i])
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

  while (XCheckTypedWindowEvent (dpy, w, Expose, &dummy))
    i++;
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
  extern FvwmWindow *Tmp_win; /* from events.c */
  Window child, root;
  int root_x, root_y;
  int win_x, win_y;
  Bool f = XQueryPointer(dpy, Scr.Root, &root,
                         &child, &root_x, &root_y, &win_x, &win_y, &JunkMask);
  if (f && child != None) {
    Event.xany.window = child;
    if (XFindContext(dpy, child, FvwmContext, (caddr_t *) &Tmp_win) == XCNOENT)
      Tmp_win = NULL;
    HandleEnterNotify();
    Tmp_win = None;
  }
}
