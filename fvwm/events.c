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
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include "fvwm.h"
#include "icons.h"
#include <X11/Xatom.h>
#include "misc.h"
#include "bindings.h"
#include "parse.h"
#include "screen.h"
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */
#include "module.h"
#include "session.h"

#ifndef XUrgencyHint
#define XUrgencyHint            (1L << 8)
#endif

extern int menuFromFrameOrWindowOrTitlebar;

extern Boolean debugging;
extern Bool fFvwmInStartup;

extern void StartupStuff(void);

int Context = C_NO_CONTEXT;	/* current button press context */
int Button = 0;
FvwmWindow *ButtonWindow;	/* button press window structure */
XEvent Event;			/* the current event */
FvwmWindow *Tmp_win;		/* the current fvwm window */

int last_event_type=0;
Window last_event_window=0;

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
void  ResyncFvwmStackRing(void);
void BroadcastRestack(FvwmWindow *,FvwmWindow *);

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
}

/***********************************************************************
 *
 *  Procedure:
 *	DispatchEvent - handle a single X event stored in global var Event
 *
 ************************************************************************/
void DispatchEvent(void)
{
  Window w = Event.xany.window;

  DBUG("DispatchEvent","Routine Entered");

  StashEventTime(&Event);

  XFlush(dpy);
  if (XFindContext (dpy, w, FvwmContext, (caddr_t *) &Tmp_win) == XCNOENT)
    Tmp_win = NULL;
  last_event_type = Event.type;
  last_event_window = w;

  if (EventHandlerJumpTable[Event.type]) {
    /* clear the rubber band outline, this is NOP if it doesn't exist */
    MoveOutline(Scr.Root, 0, 0, 0, 0);
    (*EventHandlerJumpTable[Event.type])();
  }

#ifdef C_ALLOCA
  /* If we're using the C version of alloca, see if anything needs to be
   * freed up.
   */
  alloca(0);
#endif

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
  /*
   * TEMPORARY declaration: this variable should really be
   * part of a separate source-file to handle signals
   */
  extern volatile sig_atomic_t isTerminated;
  /**/

  DBUG("HandleEvents","Routine Entered");
  while ( !isTerminated )
    {
      last_event_type = 0;
      if(My_XNextEvent(dpy, &Event))
	{
	  DispatchEvent ();
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

  if(!t)
    return C_ROOT;

  Context = C_NO_CONTEXT;
  *w= e->xany.window;

  if(*w == Scr.NoFocusWin)
    return C_ROOT;

  /* Since key presses and button presses are grabbed in the frame
   * when we have re-parented windows, we need to find out the real
   * window where the event occured */
#if 0
  /* domivogt (2-Jan-1999): Causes a bug with ClickToFocus.
   * keys and buttons are treated differently here because keys are bound to
   * the frame window and buttons are bound to the client window (with
   * XGrabKey/XGrabButton). */
  if((e->type == KeyPress)&&(e->xkey.subwindow != None))
    *w = e->xkey.subwindow;

  if((e->type == ButtonPress)&&(e->xbutton.subwindow != None)&&
     ((e->xbutton.subwindow == t->w)||(e->xbutton.subwindow == t->Parent)))
    *w = e->xbutton.subwindow;
#else
  if(e->xkey.subwindow != None)
    {
      if (e->type == KeyPress)
	*w = e->xkey.subwindow;
      else if ((*w != t->w && *w != t->Parent) ||
	       e->xbutton.subwindow == t->w ||
	       e->xbutton.subwindow == t->Parent)
	/* domivogt (6-Jan-198): I don't understand what's happening here. If
	 * the mouse is over the client window. The subwindow has an unique id
	 * that no visible part of the FvwmWindow has. */
	*w = e->xbutton.subwindow;
    }
#endif

  if (*w == Scr.Root)
    Context = C_ROOT;
  if (t)
    {
      if (*w == t->title_w)
	Context = C_TITLE;
      else if ((*w == t->w)||(*w == t->Parent))
	Context = C_WINDOW;
      else if (*w == t->icon_w || *w == t->icon_pixmap_w)
	Context = C_ICON;
      else if (*w == t->frame)
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
	    for(i=0;i<Scr.nr_left_buttons;i++)
	      {
		if(*w == t->left_w[i])
		  {
		    Context = (1<<i)*C_L1;
		    break;
		  }
	      }
	    if (i < Scr.nr_left_buttons)
	      Button = i;
	    else
	      {
		for(i=0;i<Scr.nr_right_buttons;i++)
		  {
		    if(*w == t->right_w[i])
		      {
			Context = (1<<i)*C_R1;
			Button = i;
			break;
		      }
		  }
	      }
	  } /* if (i < 4) */
      } /* else */
    } /* if (t) */
  return Context;
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleFocusIn - handles focus in events
 *
 ************************************************************************/
void HandleFocusIn(void)
{
  XEvent d;
  Window w;

  DBUG("HandleFocusIn","Routine Entered");

  w= Event.xany.window;
  while(XCheckTypedEvent(dpy,FocusIn,&d))
    {
      w = d.xany.window;
    }
  if (XFindContext (dpy, w, FvwmContext, (caddr_t *) &Tmp_win) == XCNOENT)
    {
      Tmp_win = NULL;
    }

  if(!Tmp_win)
    {
      if(w != Scr.NoFocusWin)
	{
	  Scr.UnknownWinFocused = w;
	}
      else
	{
	  SetBorder(Scr.Hilite,False,True,True,None);
	  BroadcastPacket(M_FOCUS_CHANGE, 5,
                          0, 0, (unsigned long)IsLastFocusSetByMouse(),
                          Scr.DefaultDecor.HiColors.fore,
                          Scr.DefaultDecor.HiColors.back);
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
    }
  else if(Tmp_win != Scr.Hilite)
    {
      SetBorder(Tmp_win,True,True,True,None);
      BroadcastPacket(M_FOCUS_CHANGE, 5,
                      Tmp_win->w, Tmp_win->frame,
		      (unsigned long)IsLastFocusSetByMouse(),
                      GetDecor(Tmp_win,HiColors.fore),
                      GetDecor(Tmp_win,HiColors.back));
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

  ButtonWindow = Tmp_win;

  DBUG("HandleKeyPress","Routine Entered");

  Context = GetContext(Tmp_win,&Event, &PressedW);
  PressedW = None;

  /* Here's a real hack - some systems have two keys with the
   * same keysym and different keycodes. This converts all
   * the cases to one keycode. */
  Event.xkey.keycode =
    XKeysymToKeycode(dpy,XKeycodeToKeysym(dpy,Event.xkey.keycode,0));

  /* Check if there is something bound to the key */
  action = CheckBinding(Scr.AllBindings, Event.xkey.keycode,
			Event.xkey.state, GetUnusedModifiers(), Context,
			KEY_BINDING);
  if (action != NULL)
    {
      ExecuteFunction(action,Tmp_win, &Event, Context, -1, EXPAND_COMMAND);
      return;
    }

  /* if we get here, no function key was bound to the key.  Send it
   * to the client if it was in a window we know about.
   */
  if (Tmp_win)
    {
      if(Event.xkey.window != Tmp_win->w)
	{
	  Event.xkey.window = Tmp_win->w;
	  XSendEvent(dpy, Tmp_win->w, False, KeyPressMask, &Event);
	}
    }

  ButtonWindow = NULL;
}


/***********************************************************************
 *
 *  Procedure:
 *	HandlePropertyNotify - property notify event handler
 *
 ***********************************************************************/
#define MAX_NAME_LEN 200L		/* truncate to this many */
#define MAX_ICON_NAME_LEN 200L		/* ditto */

void HandlePropertyNotify(void)
{
  XTextProperty text_prop;
  Boolean       OnThisPage    =  False;
  int old_wmhints_flags;

  DBUG("HandlePropertyNotify","Routine Entered");

  if ((!Tmp_win)||
      (XGetGeometry(dpy, Tmp_win->w, &JunkRoot, &JunkX, &JunkY,
		    &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0))
    return;

  /*
      Make sure at least part of window is on this page
      before giving it focus...
  */
  if ( (Tmp_win->Desk == Scr.CurrentDesk) &&
       ( ((Tmp_win->frame_x + Tmp_win->frame_width) >= 0 &&
          Tmp_win->frame_x < Scr.MyDisplayWidth) &&
         ((Tmp_win->frame_y + Tmp_win->frame_height) >= 0 &&
          Tmp_win->frame_y < Scr.MyDisplayHeight)
       )
     )
    {
      OnThisPage  =  True;
    }

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
      if (!XGetWMName(dpy, Tmp_win->w, &text_prop))
	return;

      free_window_names (Tmp_win, True, False);

      Tmp_win->name = (char *)text_prop.value;
#ifdef SESSION
      SET_NAME_CHANGED(Tmp_win, 1);
#endif
      if (Tmp_win->name == NULL)
        Tmp_win->name = NoName;
      BroadcastName(M_WINDOW_NAME,Tmp_win->w,Tmp_win->frame,
		    (unsigned long)Tmp_win,Tmp_win->name);

      /* fix the name in the title bar */
      if(!IS_ICONIFIED(Tmp_win))
	SetTitleBar(Tmp_win,(Scr.Hilite==Tmp_win),True);

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
      if (!XGetWMIconName (dpy, Tmp_win->w, &text_prop))
	return;
      free_window_names (Tmp_win, False, True);
      Tmp_win->icon_name = (char *) text_prop.value;
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
	    Tmp_win->icon_bitmap_file = Scr.DefaultIcon;

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
			       Tmp_win->icon_x_loc,Tmp_win->icon_y_loc);
	      BroadcastPacket(M_ICONIFY, 7,
			      Tmp_win->w, Tmp_win->frame,
			      (unsigned long)Tmp_win,
			      Tmp_win->icon_x_loc, Tmp_win->icon_y_loc,
			      Tmp_win->icon_w_width, Tmp_win->icon_w_height);
	      BroadcastConfig(M_CONFIGURE_WINDOW, Tmp_win);

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
	  ExecuteFunction("Function UrgencyFunc",
			  Tmp_win,&Event,C_WINDOW,-1,EXPAND_COMMAND);
	}

      if ((old_wmhints_flags & XUrgencyHint) &&
	  !(Tmp_win->wmhints->flags & XUrgencyHint))
	{
	  ExecuteFunction("Function UrgencyDoneFunc",
			  Tmp_win,&Event,C_WINDOW,-1,EXPAND_COMMAND);
	}
      break;
    case XA_WM_NORMAL_HINTS:
      GetWindowSizeHints (Tmp_win);
#if 0
      /*
      ** ckh - not sure why this next stuff was here, but fvwm 1.xx
      ** didn't do this, and it seems to cause a bug when changing
      ** fonts in XTerm
      */
      {
	int new_width, new_height;
	new_width = Tmp_win->frame_width;
	new_height = Tmp_win->frame_height;
	ConstrainSize(Tmp_win, &new_width, &new_height, False, 0, 0);
	if((new_width != Tmp_win->frame_width)||
	   (new_height != Tmp_win->frame_height))
	  SetupFrame(Tmp_win,Tmp_win->frame_x, Tmp_win->frame_y,
		     new_width,new_height,False,False);
      }
#endif /* 0 */
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
	        SetFocus(Tmp_win->w,Tmp_win,0);
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

#ifdef GNOME
  /* Process GNOME Messages */
  if (GNOME_ProcessClientMessage(Tmp_win, &Event))
    {
      return;
    }
#endif

  if ((Event.xclient.message_type == _XA_WM_CHANGE_STATE)&&
      (Tmp_win)&&(Event.xclient.data.l[0]==IconicState)&&
      !IS_ICONIFIED(Tmp_win))
  {
    XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
                   &(button.xmotion.x_root),
                   &(button.xmotion.y_root),
                   &JunkX, &JunkY, &JunkMask);
    button.type = 0;
    ExecuteFunction("Iconify",Tmp_win, &button,C_FRAME,-1, EXPAND_COMMAND);
    return;
  }

  /* FIXME: Is this safe enough ? I guess if clients behave
     according to ICCCM and send these messages only if they
     when grabbed the pointer, it is OK */
  {
    extern Bool client_controls_colormaps;
    extern Atom _XA_WM_COLORMAP_NOTIFY;
    if (Event.xclient.message_type == _XA_WM_COLORMAP_NOTIFY) {
      client_controls_colormaps = Event.xclient.data.l[1];
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
      if ((Event.xany.window == Tmp_win->title_w))
	{
	  SetTitleBar(Tmp_win,(Scr.Hilite == Tmp_win),False);
	}
      else
	{
	  SetBorder(Tmp_win,(Scr.Hilite == Tmp_win),True,True,
		    Event.xany.window);
	}
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

  Destroy(Tmp_win);

#ifdef GNOME
  GNOME_SetClientList();
#endif
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
void HandleMapRequestKeepRaised(Window KeepRaised,  FvwmWindow  *ReuseWin)
{
  extern long isIconicState;
  extern Boolean PPosOverride;
  Boolean       OnThisPage    =  False;

  Event.xany.window = Event.xmaprequest.window;

  if (ReuseWin == NULL)
    {
      if(XFindContext(dpy, Event.xany.window, FvwmContext,
		      (caddr_t *)&Tmp_win)==XCNOENT)
      Tmp_win = NULL;
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
      Make sure at least part of window is on this page
      before giving it focus...
  */
  if ( (Tmp_win->Desk == Scr.CurrentDesk) &&
       ( ((Tmp_win->frame_x + Tmp_win->frame_width) >= 0 &&
          Tmp_win->frame_x < Scr.MyDisplayWidth) &&
         ((Tmp_win->frame_y + Tmp_win->frame_height) >= 0 &&
          Tmp_win->frame_y < Scr.MyDisplayHeight)
       )
     )
    {
      OnThisPage  =  True;
    }

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
	      XMapWindow(dpy, Tmp_win->w);
	      XMapWindow(dpy, Tmp_win->frame);
	      SET_MAP_PENDING(Tmp_win, 1);
	      SetMapStateProp(Tmp_win, NormalState);
	      if((!Scr.Focus) ||
		 (!IS_TRANSIENT(Tmp_win) && DO_GRAB_FOCUS(Tmp_win)) ||
		 (IS_TRANSIENT(Tmp_win) && DO_GRAB_FOCUS_TRANSIENT(Tmp_win)))
		{
                  if (OnThisPage && !HAS_NEVER_FOCUS(Tmp_win))
                    {
		      SetFocus(Tmp_win->w, Tmp_win, 1);
                    }
		}
	    }
	  else
	    {
	      XMapWindow(dpy, Tmp_win->w);
	      SetMapStateProp(Tmp_win, NormalState);
	    }
	  break;

	case IconicState:
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

#ifdef SESSION
  /* just to be on the safe side, we make sure that STARTICONIC
     can only influence the initial transition from withdrawn state */
  SET_DO_START_ICONIC(Tmp_win, 0);
#endif

#ifdef GNOME
  GNOME_SetClientList();
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
  Boolean       OnThisPage    =  False;

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
   * don't need or want windows associated with the sunstructurenotifymask */
  if(Event.xmap.event != Event.xmap.window)
    return;

  /*
      Make sure at least part of window is on this page
      before giving it focus...
  */
  if ( (Tmp_win->Desk == Scr.CurrentDesk) &&
       ( ((Tmp_win->frame_x + Tmp_win->frame_width) >= 0 &&
          Tmp_win->frame_x < Scr.MyDisplayWidth) &&
         ((Tmp_win->frame_y + Tmp_win->frame_height) >= 0 &&
          Tmp_win->frame_y < Scr.MyDisplayHeight)
       )
     )
    {
      OnThisPage  =  True;
    }

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

  if((!Scr.Focus) ||
     (!IS_TRANSIENT(Tmp_win) && DO_GRAB_FOCUS(Tmp_win)) ||
     (IS_TRANSIENT(Tmp_win) && DO_GRAB_FOCUS_TRANSIENT(Tmp_win)))
    {
      if (OnThisPage && !HAS_NEVER_FOCUS(Tmp_win))
        {
          SetFocus(Tmp_win->w,Tmp_win,1);
        }
    }
  if((!(HAS_BORDER(Tmp_win)|HAS_TITLE(Tmp_win)))&&(Tmp_win->boundary_width <2))
    {
      SetBorder(Tmp_win,False,True,True,Tmp_win->frame);
    }
  XSync(dpy,0);
  MyXUngrabServer (dpy);
  XFlush (dpy);
  SET_MAPPED(Tmp_win, 1);
  SET_MAP_PENDING(Tmp_win, 0);
  SET_ICONIFIED(Tmp_win, 0);
  SET_ICON_UNMAPPED(Tmp_win, 0);
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
  int    focus_grabbed;

  DBUG("HandleUnmapNotify","Routine Entered");

  /*
   * Don't ignore events as described below.
   */
  if((Event.xunmap.event != Event.xunmap.window) &&
     (Event.xunmap.event != Scr.Root || !Event.xunmap.send_event))
    {
      return;
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

  if(weMustUnmap)
    XUnmapWindow(dpy, Event.xunmap.window);

  if(Tmp_win ==  Scr.Hilite)
    Scr.Hilite = NULL;

  if(Scr.PreviousFocus == Tmp_win)
    Scr.PreviousFocus = NULL;

  focus_grabbed = (Tmp_win == Scr.Focus) &&
    ((!IS_TRANSIENT(Tmp_win) && DO_GRAB_FOCUS(Tmp_win)) ||
     (IS_TRANSIENT(Tmp_win) && DO_GRAB_FOCUS_TRANSIENT(Tmp_win))) &&
    !HAS_NEVER_FOCUS(Tmp_win);

  if((Tmp_win == Scr.Focus)&&(HAS_CLICK_FOCUS(Tmp_win)))
    {
      if(Tmp_win->next)
	{
	  HandleHardFocus(Tmp_win->next);
	}
      else
	SetFocus(Scr.NoFocusWin,NULL,1);
    }

  if(Scr.Focus == Tmp_win)
    SetFocus(Scr.NoFocusWin,NULL,1);

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
      Destroy(Tmp_win);
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
      Destroy(Tmp_win);		/* do not need to mash event before */
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

#ifdef GNOME
  GNOME_SetClientList();
#endif
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

  DBUG("HandleButtonPress","Routine Entered");

  /* click to focus stuff goes here */
  if((Tmp_win)&&(HAS_CLICK_FOCUS(Tmp_win))&&(Tmp_win != Scr.Ungrabbed))
  {
    SetFocus(Tmp_win->w,Tmp_win,1);
    if (Scr.go.ClickToFocusRaises ||
	((Event.xany.window != Tmp_win->w)&&
	 (Event.xbutton.subwindow != Tmp_win->w)&&
	 (Event.xany.window != Tmp_win->Parent)&&
	 (Event.xbutton.subwindow != Tmp_win->Parent)))
    {
      RaiseWindow(Tmp_win);
    }

    if(!IS_ICONIFIED(Tmp_win))
    {
      XSync(dpy,0);
      /* pass click event to just clicked to focus window? Do not swallow the
       * click if the window didn't accept the focus */
      if (Scr.go.ClickToFocusPassesClick || Scr.Focus != Tmp_win)
	XAllowEvents(dpy,ReplayPointer,CurrentTime);
      else /* don't pass click to just focused window */
	XAllowEvents(dpy,AsyncPointer,CurrentTime);
      XSync(dpy,0);
      return;
    }
  }
  else if ((Tmp_win) && !(HAS_CLICK_FOCUS(Tmp_win)) &&
           (Event.xbutton.window == Tmp_win->frame) &&
	   Scr.go.MouseFocusClickRaises)
  {
    if (Tmp_win != Scr.LastWindowRaised &&
        MaskUsedModifiers(Event.xbutton.state) == 0 &&
        GetContext(Tmp_win,&Event, &PressedW) == C_WINDOW)
    {
      RaiseWindow(Tmp_win);
      XSync(dpy,0);
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
      XSync(dpy,0);
      return;
    }
  }

  XSync(dpy,0);
  XAllowEvents(dpy,ReplayPointer,CurrentTime);
  XSync(dpy,0);

  Context = GetContext(Tmp_win,&Event, &PressedW);
  LocalContext = Context;
  if(Context == C_TITLE)
    SetTitleBar(Tmp_win,(Scr.Hilite == Tmp_win),False);
  else if ((Context & C_LALL) || (Context & C_RALL))
    SetBorder(Tmp_win, (Scr.Hilite == Tmp_win), True, True, PressedW);
  else
    SetBorder(Tmp_win, (Scr.Hilite == Tmp_win), True, True, None);

  ButtonWindow = Tmp_win;

  /* we have to execute a function or pop up a menu */
  /* need to search for an appropriate mouse binding */
  action = CheckBinding(Scr.AllBindings, Event.xbutton.button,
			Event.xbutton.state, GetUnusedModifiers(), Context,
			MOUSE_BINDING);
  if (action != NULL)
    ExecuteFunction(action,Tmp_win, &Event, Context, -1, EXPAND_COMMAND);

  OldPressedW = PressedW;
  PressedW = None;
  if(LocalContext == C_TITLE)
    SetTitleBar(ButtonWindow,(Scr.Hilite==ButtonWindow),False);
  else if ((Context & C_LALL) || (Context & C_RALL))
    SetBorder(ButtonWindow,(Scr.Hilite == ButtonWindow),True,True,OldPressedW);
  else
    SetBorder(ButtonWindow,(Scr.Hilite == ButtonWindow),True,True,
	      Tmp_win ? Tmp_win->frame : 0);
  ButtonWindow = NULL;
}

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

  /* look for a matching leaveNotify which would nullify this enterNotify */
  if(XCheckTypedWindowEvent (dpy, ewp->window, LeaveNotify, &d))
    {
      /*
         RBW - if we're in startup, this is a coerced focus, so we don't
         want to save the event time, or exit prematurely.
      */
      if (! fFvwmInStartup)
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
                 &Event.xcrossing.x_root,&Event.xcrossing.y_root,
                 &delta_x,&delta_y,True,True);
    return;
  }

  /* multi screen? */
  if (Event.xany.window == Scr.Root)
  {
    if (!Scr.Focus || HAS_MOUSE_FOCUS(Scr.Focus))
    {
      SetFocus(Scr.NoFocusWin,NULL,1);
    }
    if (Scr.ColormapFocus == COLORMAP_FOLLOWS_MOUSE)
    {
      InstallWindowColormaps(NULL);
    }
    return;
  }

  /* make sure its for one of our windows */
  if (!Tmp_win)
    return;

  if(HAS_MOUSE_FOCUS(Tmp_win) || HAS_SLOPPY_FOCUS(Tmp_win))
    {
      SetFocus(Tmp_win->w,Tmp_win,1);
    }
  if (Scr.ColormapFocus == COLORMAP_FOLLOWS_MOUSE)
  {
    if((!IS_ICONIFIED(Tmp_win))&&(Event.xany.window == Tmp_win->w))
      InstallWindowColormaps(Tmp_win);
    else
      InstallWindowColormaps(NULL);
  }

  if (IS_ICONIFIED(Tmp_win))
    {
      SET_ICON_ENTERED(Tmp_win,1);
      DrawIconWindow (Tmp_win);
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
  if(Event.xcrossing.window == Scr.Root)
    {
      if(Event.xcrossing.mode == NotifyNormal)
	{
	  if (Event.xcrossing.detail != NotifyInferior)
	    {
	      if(Scr.Focus != NULL)
		{
		  SetFocus(Scr.NoFocusWin,NULL,1);
		}
	      if(Scr.Hilite != NULL)
		SetBorder(Scr.Hilite,False,True,True,None);
	    }
	}
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
  XWindowChanges xwc;
  unsigned long xwcm;
  int x, y, width, height;
  XConfigureRequestEvent *cre = &Event.xconfigurerequest;
  Bool sendEvent=False;
  FvwmWindow  *FvwmSib;

  DBUG("HandleConfigureRequest","Routine Entered");

  /*
   * Event.xany.window is Event.xconfigurerequest.parent, so Tmp_win will
   * be wrong
   */
  Event.xany.window = cre->window;	/* mash parent field */
  if (XFindContext (dpy, cre->window, FvwmContext, (caddr_t *) &Tmp_win) ==
      XCNOENT)
    Tmp_win = NULL;

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
      Tmp_win->icon_x_loc = cre->x +
        (Tmp_win->icon_w_width - Tmp_win->icon_p_width)/2;
      Tmp_win->icon_y_loc = cre->y - Tmp_win->icon_p_height;
      if(!IS_ICON_UNMAPPED(Tmp_win))
        BroadcastPacket(M_ICON_LOCATION, 7,
                        Tmp_win->w, Tmp_win->frame,
                        (unsigned long)Tmp_win,
                        Tmp_win->icon_x_loc, Tmp_win->icon_y_loc,
                        Tmp_win->icon_w_width,
                        Tmp_win->icon_w_height + Tmp_win->icon_p_height);
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
	xwc.x = Tmp_win->icon_x_loc;
	xwc.y = Tmp_win->icon_y_loc - Tmp_win->icon_p_height;
	xwcm = cre->value_mask & (CWX | CWY);
	XConfigureWindow(dpy, Tmp_win->icon_pixmap_w, xwcm, &xwc);
      }
      if(Tmp_win->icon_w != None)
      {
	xwc.x = Tmp_win->icon_x_loc;
	xwc.y = Tmp_win->icon_y_loc;
	xwcm = cre->value_mask & (CWX | CWY);
        XConfigureWindow(dpy, Tmp_win->icon_w, xwcm, &xwc);
      }
    }
    return;
  }

  /*  Stacking order change requested...  */
  if (cre->value_mask & CWStackMode)
  {
    FvwmWindow *otherwin;

    otherwin  =  NULL;
    xwc.sibling = (((cre->value_mask & CWSibling) &&
                    (XFindContext (dpy, cre->above, FvwmContext,
                                   (caddr_t *) &otherwin) == XCSUCCESS))
                   ? otherwin->frame : cre->above);
     /* we only allow clients to restack their windows within
       their layer. Thus we don't handle TopIf, BottomIf or
       Opposite modes. We could perhaps allow them and then
       catch windows that have gone off to the end of the stack
       and bring them back to the layer, but who cares ? Nobody will
       use these modes anyway.
    */
    if (!(cre->value_mask & CWSibling))
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
    else if (otherwin &&
      (otherwin->layer == Tmp_win->layer) &&
      ((cre->detail == Above) || (cre->detail == Below)))
      {
    xwc.stack_mode = cre->detail;
    XConfigureWindow (dpy, Tmp_win->frame,
                      cre->value_mask & (CWSibling | CWStackMode), &xwc);

    xwc.sibling = Tmp_win->frame;
    xwc.stack_mode = Below;
      if (Tmp_win->icon_w != None)
	{
	  XConfigureWindow(dpy,Tmp_win->icon_w,CWSibling|CWStackMode,&xwc);
	}
      if (Tmp_win->icon_pixmap_w != None)
	{
	  XConfigureWindow(dpy,Tmp_win->icon_pixmap_w,CWSibling|CWStackMode,&xwc);
	}

    sendEvent = True;

    /*
        RBW - Update the stacking order ring.
    */
    if (xwc.stack_mode == Above || xwc.stack_mode == Below)
      {
        FvwmSib = (otherwin != NULL ) ? otherwin: Scr.FvwmRoot.stack_next;  /*  Set up for Above.  */
        if (xwc.stack_mode == Below)
	  {
            /*
                If Below-sibling, raise above next lower window. If no sibling,
                bottom of stack is "above" Scr.FvwmRoot in the ring.
            */
            FvwmSib = (FvwmSib == otherwin) ? FvwmSib->stack_next: FvwmSib->stack_prev;
	  }
        if (Tmp_win != FvwmSib)                            /* Don't chain it to itself!  */
          {
            Tmp_win->stack_prev->stack_next = Tmp_win->stack_next;  /* Pluck from chain.   */
            Tmp_win->stack_next->stack_prev = Tmp_win->stack_prev;
            Tmp_win->stack_next = FvwmSib;                          /* Set new pointers.   */
            Tmp_win->stack_prev = FvwmSib->stack_prev;
            FvwmSib->stack_prev->stack_next = Tmp_win;              /* Re-insert above sibling. */
            FvwmSib->stack_prev = Tmp_win;
          }
      }
    else
      {
        /*
            Oh, bother! We have to rebuild the stacking order ring to figure
            out where this one went (TopIf, BottomIf, or Opposite).
        */
        ResyncFvwmStackRing();
      }

     /*
         Let the modules know that Tmp_win changed its place
         in the stacking order
      */
     BroadcastRestack (Tmp_win->stack_prev, Tmp_win->stack_next);
  }
  }

#ifdef SHAPE
  if (ShapesSupported)
  {
    int xws, yws, xbs, ybs;
    unsigned wws, hws, wbs, hbs;
    int boundingShaped, clipShaped;

    XShapeQueryExtents (dpy, Tmp_win->w,&boundingShaped, &xws, &yws, &wws,
			&hws,&clipShaped, &xbs, &ybs, &wbs, &hbs);
    Tmp_win->wShaped = boundingShaped;
  }
#endif /* SHAPE */

  /* Don't modify frame_XXX fields before calling SetupWindow! */
  x = Tmp_win->frame_x;
  y = Tmp_win->frame_y;
  width = Tmp_win->frame_width;
  height = Tmp_win->frame_height;

  /* for restoring */
  if (cre->value_mask & CWBorderWidth)
  {
    Tmp_win->old_bw = cre->border_width;
  }
  /* override even if border change */

  if (cre->value_mask & CWX)
    x = cre->x - Tmp_win->boundary_width;
  if (cre->value_mask & CWY)
    y = cre->y - Tmp_win->boundary_width - Tmp_win->title_height;
  if (cre->value_mask & CWWidth)
    width = cre->width + 2*Tmp_win->boundary_width;
  if (cre->value_mask & CWHeight)
    height = cre->height+Tmp_win->title_height+2*Tmp_win->boundary_width;

  /*
   * SetupWindow (x,y) are the location of the upper-left outer corner and
   * are passed directly to XMoveResizeWindow (frame).  The (width,height)
   * are the inner size of the frame.  The inner width is the same as the
   * requested client window width; the inner height is the same as the
   * requested client window height plus any title bar slop.
   */
  ConstrainSize(Tmp_win, &width, &height, False, 0, 0);
  if (IS_SHADED(Tmp_win))
    {
      /* for shaded windows, allow resizing, but keep it shaded */
      SetupFrame (Tmp_win, x, y, width, Tmp_win->frame_height,sendEvent,False);
      Tmp_win->orig_ht = height;
    }
  else if (!IS_MAXIMIZED(Tmp_win))
    {
      /* dont allow clients to resize maximized windows */
      SetupFrame (Tmp_win, x, y, width, height, sendEvent, False);
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
    SetShape(Tmp_win,Tmp_win->frame_width);
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
	SET_VISIBLE(Tmp_win, 1);
      else
	SET_VISIBLE(Tmp_win, 0);
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
  extern int fd_width, x_fd;
  fd_set in_fdset, out_fdset;
  Window targetWindow;
  int i;
  static struct timeval timeout = {42, 0};
  static struct timeval *timeoutP = &timeout;

  DBUG("My_XNextEvent","Routine Entered");

  if(XPending(dpy)) {
    DBUG("My_XNextEvent","taking care of queued up events & returning");
    XNextEvent(dpy,event);
    StashEventTime(event);
    return 1;
  }

  DBUG("My_XNextEvent","no X events waiting - about to reap children");
  /* Zap all those zombies! */
  /* If we get to here, then there are no X events waiting to be processed.
   * Just take a moment to check for dead children. */
  ReapChildren();

  /* check for termination of all startup modules */
  if (fFvwmInStartup) {
    for(i=0;i<npipes;i++)
      if (FD_ISSET(i, &init_fdset))
        break;
#if 0
    if (i == npipes) {
#else
    if (i == npipes || writePipes[i+1] == 0) {
#endif
      DBUG("My_XNextEvent", "Starting up after command lines modules\n");
      StartupStuff();
      timeoutP = NULL; /* set an infinite timeout to stop ticking */
    }
  }

  FD_ZERO(&in_fdset);
  FD_SET(x_fd,&in_fdset);
#ifdef SESSION
  if (sm_fd >= 0) FD_SET(sm_fd, &in_fdset);
#endif
  FD_ZERO(&out_fdset);
  for(i=0; i<npipes; i++) {
    if(readPipes[i]>=0)
      FD_SET(readPipes[i], &in_fdset);
    if(pipeQueue[i]!= NULL)
      FD_SET(writePipes[i], &out_fdset);
  }

  DBUG("My_XNextEvent","waiting for module input/output");
  XFlush(dpy);
  if (select((SELECT_TYPE_ARG1)fd_width,
             SELECT_TYPE_ARG234 &in_fdset,
             SELECT_TYPE_ARG234 &out_fdset,
             SELECT_TYPE_ARG234 0,
             SELECT_TYPE_ARG5   timeoutP) > 0) {

    /* Check for module input. */
    for (i=0; i<npipes; i++) {
      if ((readPipes[i] >= 0) && FD_ISSET(readPipes[i], &in_fdset)) {
        if (read(readPipes[i], &targetWindow, sizeof(Window)) > 0) {
          DBUG("My_XNextEvent","calling HandleModuleInput");
          HandleModuleInput(targetWindow,i);
	} else {
          DBUG("My_XNextEvent","calling KillModule");
          KillModule(i,10);
        }
      }
      if ((writePipes[i] >= 0) && FD_ISSET(writePipes[i], &out_fdset)) {
        DBUG("My_XNextEvent","calling FlushQueue");
        FlushQueue(i);
      }
    }

#ifdef SESSION
    if ((sm_fd >= 0) && (FD_ISSET(sm_fd, &in_fdset))) ProcessICEMsgs();
#endif

  } else {
    /* select has timed out, things must have calmed down so let's decorate */
    if (fFvwmInStartup) {
      fvwm_msg(ERR, "My_XNextEvent",
               "Some command line modules have not quit, Starting up after timeout.\n");
      StartupStuff();
      timeoutP = NULL; /* set an infinite timeout to stop ticking */
    }
  }

  DBUG("My_XNextEvent","leaving My_XNextEvent");
  return 0;
}




/*
    RBW - 01/07/1998  -  this is here temporarily - I mean to move it to
    libfvwm eventually, along with some other chain manipulation functions.
*/

/*
    ResyncFvwmStackRing -
    Rebuilds the stacking order ring of FVWM-managed windows. For use in cases
    where apps raise/lower their own windows in a way that makes it difficult
    to determine exactly where they ended up in the stacking order.
    - Based on code from Matthias Clasen.
*/
void  ResyncFvwmStackRing (void)
{
  Window root, parent, *children;
  unsigned int nchildren, i;
  FvwmWindow *t1, *t2;

  MyXGrabServer (dpy);

  if (!XQueryTree (dpy, Scr.Root, &root, &parent, &children, &nchildren))
    {
      MyXUngrabServer (dpy);
      return;
    }

  t2 = &Scr.FvwmRoot;
  for (i = 0; i < nchildren; i++)
    {
      for (t1 = Scr.FvwmRoot.next; t1 != NULL; t1 = t1->next)
	{
          if (IS_ICONIFIED(t1) && !IS_ICON_SUPPRESSED(t1))
            {
	      if (t1->icon_w == children[i])
	        {
	          break;
	        }
              else if (t1->icon_pixmap_w == children[i])
	        {
	          break;
	        }
            }
          else
            {
	      if (t1->frame == children[i])
	        {
	          break;
	        }
            }
	}

      if (t1 != NULL && t1 != t2)
	{
          /*
              Move the window to its new position, working from the bottom up
              (that's the way XQueryTree presents the list).
          */
          t1->stack_prev->stack_next = t1->stack_next;  /* Pluck from chain. */
          t1->stack_next->stack_prev = t1->stack_prev;
          t1->stack_next = t2;                          /* Set new pointers. */
          t1->stack_prev = t2->stack_prev;
          t2->stack_prev->stack_next = t1;              /* Insert in new
							 * position. */
          t2->stack_prev = t1;
          t2 = t1;

	}
    }

  MyXUngrabServer (dpy);

  XFree (children);
}
