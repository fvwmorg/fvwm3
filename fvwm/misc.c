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

/* To add timing info to debug output, #define this: */
#ifdef FVWM_DEBUG_TIME
#include <sys/times.h>
#endif

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
  int rep;

  /* menus.c control itself its busy stuff. No busy stuff at start up.
   * Only one busy grab */
  if ( ((GrabPointerState & (GRAB_MENU | GRAB_STARTUP))
	                 && (grab_context & GRAB_BUSY)) ||
       ((GrabPointerState & GRAB_BUSY) && (grab_context & GRAB_BUSY)) )
    return False;

  XSync(dpy,0);
  /* move the keyboard focus prior to grabbing the pointer to
   * eliminate the enterNotify and exitNotify events that go
   * to the windows. But GRAB_BUSY. */
  if (grab_context != GRAB_BUSY)
  {
    if(Scr.PreviousFocus == NULL)
      Scr.PreviousFocus = Scr.Focus;
    SetFocus(Scr.NoFocusWin,NULL,0);
    grab_win = Scr.Root;
    rep = 500;
  }
  else
  {
    if ( Scr.Hilite != NULL )
      grab_win = Scr.Hilite->w;
    else
      grab_win = Scr.Root;
    /* retry to grab the busy cursor only once */
    rep = 2;
  }
  mask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|PointerMotionMask
    | EnterWindowMask | LeaveWindowMask;
  while((i < rep)&&
	(val = XGrabPointer(
	  dpy, grab_win, True, mask, GrabModeAsync, GrabModeAsync, Scr.Root,
	  Scr.FvwmCursors[cursor], CurrentTime) != GrabSuccess))
  {
    switch (val)
    {
    case GrabInvalidTime:
    case GrabNotViewable:
      /* give up */
      i += 100000;
      break;
    case GrabSuccess:
      break;
    case AlreadyGrabbed:
    case GrabFrozen:
    default:
      /* If you go too fast, other windows may not get a change to release
       * any grab that they have. */
      usleep(10000);
      i++;
      break;
    }
  }

  /* If we fall out of the loop without grabbing the pointer, its
     time to give up */
  XSync(dpy,0);
  if(val!=GrabSuccess)
  {
    return False;
  }
  GrabPointerState |= grab_context;
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

  XSync(dpy,0);
  XUngrabPointer(dpy,CurrentTime);

  GrabPointerState &= ~ungrab_context;
  if(Scr.PreviousFocus != NULL && !(GRAB_BUSY & ungrab_context))
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
#ifdef FVWM_DEBUG_TIME
  clock_t time_val, time_taken;
  static clock_t start_time = 0;
  static clock_t prev_time = 0;
  struct tms not_used_tms;
  char buffer[200];                     /* oversized */
  time_t mytime;
  struct tm *t_ptr;
  static int counter = 0;
#endif

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

#ifdef FVWM_DEBUG_TIME
  time(&mytime);
  t_ptr = localtime(&mytime);
  if (start_time == 0) {
    prev_time = start_time =
      (unsigned int)times(&not_used_tms); /* get clock ticks */
  }
  time_val = (unsigned int)times(&not_used_tms); /* get clock ticks */
  time_taken = time_val - prev_time;
  prev_time = time_val;
  sprintf(buffer, "%.2d:%.2d:%.2d %ld",
          t_ptr->tm_hour, t_ptr->tm_min, t_ptr->tm_sec,time_taken);
#endif

  if (Scr.NumberOfScreens > 1)
  {
    fprintf(stderr,"[FVWM.%d][%s]: "
#ifdef FVWM_DEBUG_TIME
            "%s "
#endif
            "%s ",(int)Scr.screen,id,
#ifdef FVWM_DEBUG_TIME
            buffer,
#endif
            typestr);
  }
  else
  {
    fprintf(stderr,"[FVWM][%s]: "
#ifdef FVWM_DEBUG_TIME
            "%s "
#endif
            "%s ",id,
#ifdef FVWM_DEBUG_TIME
            buffer,
#endif
            typestr);
  }

  va_start(args,msg);
  vfprintf(stderr, msg, args);
  va_end(args);

  fprintf(stderr,"\n");
  if (type == ERR)
  {
    /* I hate to use a fixed length but this will do for now */
    char tmp[2 * MAX_TOKEN_LENGTH];
    sprintf(tmp,"[FVWM][%s]: %s ",id,typestr);
    va_start(args,msg);
    vsprintf(tmp+strlen(tmp), msg, args);
    va_end(args);
    tmp[strlen(tmp)+1]='\0';
    tmp[strlen(tmp)]='\n';
    if (strlen(tmp) >= MAX_MODULE_INPUT_TEXT_LEN)
      sprintf(tmp + MAX_MODULE_INPUT_TEXT_LEN - 5, "...\n");
    BroadcastName(M_ERROR,0,0,0,tmp);
  }

} /* fvwm_msg */
#endif


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
  fvwmlib_keyboard_shortcuts(
    dpy, Scr.screen, Event, x_move_size, y_move_size, ReturnEvent);

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

/* rounds x down to the next multiple of m */
int truncate_to_multiple (int x, int m)
{
  return (x < 0) ? (m * (((x + 1) / m) - 1)) : (m * (x / m));
}

Bool IntersectsInterval(int x1, int width1, int x2, int width2)
{
  return !(x1 + width1 <= x2 || x2 + width2 <= x1);
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

Bool move_into_rectangle(rectangle *move_rec, rectangle *target_rec)
{
  Bool has_changed = False;

  if (!IntersectsInterval(move_rec->x, move_rec->width,
                          target_rec->x, target_rec->width))
  {
    move_rec->x = move_rec->x % target_rec->width;
    if (move_rec->x < 0)
    {
      move_rec->x += target_rec->width;
    }
    move_rec->x += target_rec->x;
    has_changed = True;
  }
  if (!IntersectsInterval(move_rec->y, move_rec->height,
                          target_rec->y, target_rec->height))
  {
    move_rec->y = move_rec->y % target_rec->height;
    if (move_rec->y < 0)
    {
      move_rec->y += target_rec->height;
    }
    move_rec->y += target_rec->y;
    has_changed = True;
  }

  return has_changed;
}
