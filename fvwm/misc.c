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

/****************************************************************************
 *
 * Assorted odds and ends
 *
 **************************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <sys/wait.h>

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "module_interface.h"
#include "events.h"
#include "focus.h"

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



/****************************************************************************
 *
 * Records the time of the last processed event. Used in XSetInputFocus
 *
 ****************************************************************************/
Time lastTimestamp = CurrentTime;	/* until Xlib does this for us */

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
  if((NewTimestamp > lastTimestamp)||((lastTimestamp - NewTimestamp) > 30000))
    lastTimestamp = NewTimestamp;
  return True;
}

int GetTwoArguments(char *action, int *val1, int *val2, int *val1_unit,
		    int *val2_unit)
{
  *val1_unit = Scr.MyDisplayWidth;
  *val2_unit = Scr.MyDisplayHeight;
  return GetTwoPercentArguments(action, val1, val2, val1_unit, val2_unit);
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
  Bool AllUp = False;
  XEvent JunkEvent;
  unsigned int mask;

  while(!AllUp)
  {
    XAllowEvents(dpy,ReplayPointer,CurrentTime);
    XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild, &JunkX, &JunkY, &JunkX,
		  &JunkY, &mask);
    if(!(mask&(Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask)))
      AllUp = True;
    /* handle expose events */
    if (do_handle_expose && XPending(dpy) &&
	XCheckMaskEvent(dpy, ExposureMask, &Event))
    {
      DispatchEvent(True);
    }
  }
  XSync(dpy,0);
  while(XCheckMaskEvent(dpy,
			ButtonPressMask|ButtonReleaseMask|ButtonMotionMask,
			&JunkEvent))
  {
    StashEventTime (&JunkEvent);
    XAllowEvents(dpy,ReplayPointer,CurrentTime);
  }
}

/*****************************************************************************
 *
 * Grab the pointer and keyboard.
 * grab_context: GRAB_NORMAL, GRAB_BUSY, GRAB_MENU, GRAB_BUSYMENU.
 * GRAB_STARTUP and GRAB_NONE are used at startup but are not made
 * to be grab_context.
 ****************************************************************************/
Bool GrabEm(int cursor, int grab_context)
{
  int i=0,val=0;
  Window grab_win;
  unsigned int mask;

#ifdef BUSYCURSOR
  /* menus.c control itself its busy stuff. No busy stuff at start up.
   * Only one busy grab */
  if ( ((GrabPointerState & (GRAB_MENU | GRAB_STARTUP))
	                 && (grab_context & GRAB_BUSY)) ||
       ((GrabPointerState & GRAB_BUSY) && (grab_context & GRAB_BUSY)) )
    return False;
#endif

  XSync(dpy,0);
  /* move the keyboard focus prior to grabbing the pointer to
   * eliminate the enterNotify and exitNotify events that go
   * to the windows. But GRAB_BUSY. */
#ifdef BUSYCURSOR
  if (grab_context != GRAB_BUSY)
  {
#endif
    if(Scr.PreviousFocus == NULL)
      Scr.PreviousFocus = Scr.Focus;
    SetFocus(Scr.NoFocusWin,NULL,0);
    grab_win = Scr.Root;
#ifdef BUSYCURSOR
  }
  else
  {
    if ( Scr.Hilite != NULL )
      grab_win = Scr.Hilite->w;
    else
      grab_win = Scr.Root;
  }
#endif
  mask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|PointerMotionMask
    | EnterWindowMask | LeaveWindowMask;
  while((i < 500)&&
	(val = XGrabPointer(
	  dpy, grab_win, True, mask, GrabModeAsync, GrabModeAsync, Scr.Root,
	  Scr.FvwmCursors[cursor], CurrentTime) != GrabSuccess))
    {
      i++;
      /* If you go too fast, other windows may not get a change to release
       * any grab that they have. */
      usleep(10000);
    }

  /* If we fall out of the loop without grabbing the pointer, its
     time to give up */
  XSync(dpy,0);
  if(val!=GrabSuccess)
    {
      return False;
    }
#ifdef BUSYCURSOR
  GrabPointerState |= grab_context;
#endif
  return True;
}


/*****************************************************************************
 *
 * UnGrab the pointer and keyboard
 *
 ****************************************************************************/
void UngrabEm(int ungrab_context)
{
  Window w;

#ifdef BUSYCURSOR  
  /* menus.c control itself regarbing */
  if ((GrabPointerState & GRAB_MENU) && !(ungrab_context & GRAB_MENU))
  {
    GrabPointerState = (GrabPointerState & ~ungrab_context) | GRAB_BUSYMENU;
    return;
  }
  /* see if we need to regrab */
  if ((GrabPointerState & GRAB_BUSY) && !(ungrab_context & GRAB_BUSY))
  {
    GrabPointerState &= ~ungrab_context & ~GRAB_BUSY;
    GrabEm(CRS_WAIT, GRAB_BUSY);
    return;
  }
#endif

  XSync(dpy,0);
  XUngrabPointer(dpy,CurrentTime);

#ifdef BUSYCURSOR
  GrabPointerState &= ~ungrab_context;
  if(Scr.PreviousFocus != NULL && !(GRAB_BUSY & ungrab_context))
#else
  if(Scr.PreviousFocus != NULL)
#endif
    {
      w = Scr.PreviousFocus->w;

      /* if the window still exists, focus on it */
      if (w)
	{
	  SetFocus(w,Scr.PreviousFocus,0);
	}
      Scr.PreviousFocus = NULL;
    }
  XSync(dpy,0);
}


#ifndef fvwm_msg /* Some ports (i.e. VMS) define their own version */
/*
** fvwm_msg: used to send output from fvwm to files and or stderr/stdout
**
** type -> DBG == Debug, ERR == Error, INFO == Information, WARN == Warning
** id -> name of function, or other identifier
*/
void fvwm_msg(int type,char *id,char *msg,...)
{
  char *typestr;
  va_list args;

  switch(type)
  {
    case DBG:
#if 0
      if (!debugging)
        return;
#endif /* 0 */
      typestr="<<DEBUG>>";
      break;
    case ERR:
      typestr="<<ERROR>>";
      break;
    case WARN:
      typestr="<<WARNING>>";
      break;
    case INFO:
    default:
      typestr="";
      break;
  }

  va_start(args,msg);

  fprintf(stderr,"[FVWM][%s]: %s ",id,typestr);
  vfprintf(stderr, msg, args);
  fprintf(stderr,"\n");

  if (type == ERR)
  {
    char tmp[1024]; /* I hate to use a fixed length but this will do for now */
    sprintf(tmp,"[FVWM][%s]: %s ",id,typestr);
    vsprintf(tmp+strlen(tmp), msg, args);
    tmp[strlen(tmp)+1]='\0';
    tmp[strlen(tmp)]='\n';
    BroadcastName(M_ERROR,0,0,0,tmp);
  }

  va_end(args);
} /* fvwm_msg */
#endif


/* CoerceEnterNotifyOnCurrentWindow()
 * Pretends to get a HandleEnterNotify on the
 * window that the pointer currently is in so that
 * the focus gets set correctly from the beginning
 * Note that this presently only works if the current
 * window is not click_to_focus;  I think that
 * that behaviour is correct and desirable. --11/08/97 gjb */
void
CoerceEnterNotifyOnCurrentWindow(void)
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

/* Store the last item that was added with '+' */
void set_last_added_item(last_added_item_type type, void *item)
{
  Scr.last_added_item.type = type;
  Scr.last_added_item.item = item;
}

/* some fancy font handling stuff */
void NewFontAndColor(Font newfont, Pixel color, Pixel backcolor)
{
  Globalgcv.font = newfont;
  Globalgcv.foreground = color;
  Globalgcv.background = backcolor;
  Globalgcm = GCFont | GCForeground | GCBackground;
  XChangeGC(dpy,Scr.TitleGC,Globalgcm,&Globalgcv);
}

void ReapChildren(void)
{
#if HAVE_WAITPID
  while ((waitpid(-1, NULL, WNOHANG)) > 0)
    ;
#elif HAVE_WAIT3
  while ((wait3(NULL, WNOHANG, NULL)) > 0)
    ;
#else
# error One of waitpid or wait3 is needed.
#endif
}

Bool IsRectangleOnThisPage(rectangle *rec, int desk)
{
  return (desk == Scr.CurrentDesk &&
	  rec->x + rec->width > 0 &&
	  rec->x < Scr.MyDisplayWidth &&
	  rec->y + rec->height > 0 &&
	  rec->y < Scr.MyDisplayHeight) ?
    True : False;
}

/****************************************************************************
 *
 * For menus, move, and resize operations, we can effect keyboard
 * shortcuts by warping the pointer.
 *
 ****************************************************************************/
void Keyboard_shortcuts(XEvent *Event, FvwmWindow *fw, int ReturnEvent)
{
  int x_move_size = 0, y_move_size = 0;

  if (fw)
  {
    x_move_size = fw->hints.width_inc;
    y_move_size = fw->hints.height_inc;
  }
  fvwmlib_keyboard_shortcuts(dpy, Scr.screen, Event, x_move_size, y_move_size,
                             ReturnEvent);

  return;
}


/****************************************************************************
 *
 * Check if the given FvwmWindow structure still points to a valid window.
 *
 ****************************************************************************/

Bool check_if_fvwm_window_exists(FvwmWindow *fw)
{
  FvwmWindow *t;

  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    if (t == fw)
      return True;
  }
  return False;
}

/***********************************************************************
 * change by KitS@bartley.demon.co.uk to correct popups off title buttons
 *
 *  Procedure:
 *ButtonPosition - find the actual position of the button
 *                 since some buttons may be disabled
 *
 *  Returned Value:
 *The button count from left or right taking in to account
 *that some buttons may not be enabled for this window
 *
 *  Inputs:
 *      context - context as per the global Context
 *      t       - the window (FvwmWindow) to test against
 *
 ***********************************************************************/
int ButtonPosition(int context, FvwmWindow *t)
{
  int i = 0;
  int buttons = -1;
  int end = Scr.nr_left_buttons;

  if (context & C_RALL)
  {
    i  = 1;
    end = Scr.nr_right_buttons;
  }
  {
    for (; i / 2 < end; i += 2)
    {
      if (t->button_w[i])
      {
	buttons++;
      }
      /* is this the button ? */
      if (((1 << i) * C_L1) & context)
	return buttons;
    }
  }
  /* you never know... */
  return 0;
}

/* rounds x down to the next multiple of m */
int truncate_to_multiple (int x, int m)
{
  return (x < 0) ? (m * (((x + 1) / m) - 1)) : (m * (x / m));
}
