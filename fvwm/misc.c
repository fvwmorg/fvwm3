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
#include <time.h>

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

static unsigned int grab_count[GRAB_MAXVAL] = { 1, 1, 0, 0, 0, 0, 0 };
#define GRAB_EVMASK (ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask)


int GetTwoArguments(char *action, int *val1, int *val2, int *val1_unit,
		    int *val2_unit)
{
  *val1_unit = Scr.MyDisplayWidth;
  *val2_unit = Scr.MyDisplayHeight;
  return GetTwoPercentArguments(action, val1, val2, val1_unit, val2_unit);
}


/*****************************************************************************
 * Change the appearance of the grabbed cursor.
 ****************************************************************************/
static void change_grab_cursor(int cursor)
{
  if (cursor != None)
    XChangeActivePointerGrab(
      dpy, GRAB_EVMASK, Scr.FvwmCursors[cursor], CurrentTime);

  return;
}

/*****************************************************************************
 * Grab the pointer.
 * grab_context: GRAB_NORMAL, GRAB_BUSY, GRAB_MENU, GRAB_BUSYMENU,
 * GRAB_PASSIVE.
 * GRAB_STARTUP and GRAB_NONE are used at startup but are not made
 * to be grab_context.
 * GRAB_PASSIVE does not actually grab, but only delays the following ungrab
 * until the GRAB_PASSIVE is released too.
 ****************************************************************************/
#define DEBUG_GRAB 0
#if DEBUG_GRAB
void print_grab_stats(char *text)
{
  int i;

  fprintf(stderr,"grab_stats (%s):", text);
  for (i = 0; i < GRAB_MAXVAL; i++)
    fprintf(stderr," %d", grab_count[i]);
  fprintf(stderr," \n");

  return;
}
#endif
Bool GrabEm(int cursor, int grab_context)
{
  int i = 0;
  int val = 0;
  int rep;
  Window grab_win;

  if (grab_context <= GRAB_STARTUP || grab_context >= GRAB_MAXVAL)
  {
    fvwm_msg(
      ERR, "GrabEm", "Bug: Called with illegal context %d", grab_context);
    return False;
  }

  if (grab_context == GRAB_PASSIVE)
  {
    grab_count[grab_context]++;
    grab_count[GRAB_ALL]++;
    return True;
  }

  if (grab_count[GRAB_ALL] > grab_count[GRAB_PASSIVE])
  {
    /* already grabbed, just change the grab cursor */
    grab_count[grab_context]++;
    grab_count[GRAB_ALL]++;
    if (grab_context != GRAB_BUSY || grab_count[GRAB_STARTUP] == 0)
    {
      change_grab_cursor(cursor);
    }
    return True;
  }

  /* move the keyboard focus prior to grabbing the pointer to
   * eliminate the enterNotify and exitNotify events that go
   * to the windows. But GRAB_BUSY. */
  switch (grab_context)
  {
  case GRAB_BUSY:
    if ( Scr.Hilite != NULL )
      grab_win = Scr.Hilite->w;
    else
      grab_win = Scr.Root;
    /* retry to grab the busy cursor only once */
    rep = 2;
    break;
  case GRAB_PASSIVE:
    /* cannot happen */
    return False;
  default:
    grab_win = Scr.Root;
    rep = 500;
    break;
  }

  XSync(dpy,0);
  while((i < rep)&&
	(val = XGrabPointer(
	  dpy, grab_win, True, GRAB_EVMASK, GrabModeAsync, GrabModeAsync,
	  None, Scr.FvwmCursors[cursor], CurrentTime) != GrabSuccess))
  {
    switch (val)
    {
    case GrabInvalidTime:
    case GrabNotViewable:
      /* give up */
      i += rep;
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
  XSync(dpy,0);

  /* If we fall out of the loop without grabbing the pointer, its
   * time to give up */
  if (val != GrabSuccess)
  {
    return False;
  }
  grab_count[grab_context]++;
  grab_count[GRAB_ALL]++;
#if DEBUG_GRAB
print_grab_stats("grabbed");
#endif
  return True;
}


/*****************************************************************************
 *
 * UnGrab the pointer
 *
 ****************************************************************************/
Bool UngrabEm(int ungrab_context)
{
  if (ungrab_context <= GRAB_ALL || ungrab_context >= GRAB_MAXVAL)
  {
    fvwm_msg(
      ERR, "UngrabEm", "Bug: Called with illegal context %d", ungrab_context);
    return False;
  }

  if (grab_count[ungrab_context] == 0 || grab_count[GRAB_ALL] == 0)
  {
    /* context is not grabbed */
    return False;
  }

  XSync(dpy,0);
  grab_count[ungrab_context]--;
  grab_count[GRAB_ALL]--;
  if (grab_count[GRAB_ALL] > 0)
  {
    int new_cursor = None;

    /* there are still grabs left - switch grab cursor */
    switch (ungrab_context)
    {
    case GRAB_NORMAL:
    case GRAB_BUSY:
    case GRAB_MENU:
      if (grab_count[GRAB_BUSYMENU] > 0)
	new_cursor = CRS_WAIT;
      else if (grab_count[GRAB_BUSY] > 0)
	new_cursor = CRS_WAIT;
      else if (grab_count[GRAB_MENU] > 0)
	new_cursor = CRS_MENU;
      else
	new_cursor = None;
      break;
    case GRAB_BUSYMENU:
      /* switch back from busymenu cursor to normal menu cursor */
      new_cursor = CRS_MENU;
      break;
    default:
      new_cursor = None;
      break;
    }
    if (grab_count[GRAB_ALL] > grab_count[GRAB_PASSIVE])
    {
#if DEBUG_GRAB
print_grab_stats("-restore");
#endif
      change_grab_cursor(new_cursor);
    }
  }
  else
  {
#if DEBUG_GRAB
print_grab_stats("-ungrab");
#endif
    XUngrabPointer(dpy, CurrentTime);
  }
  XSync(dpy,0);

  return True;
}

#ifndef fvwm_msg /* Some ports (i.e. VMS) define their own version */
/*
** fvwm_msg: used to send output from fvwm to files and or stderr/stdout
**
** type -> DBG == Debug, ERR == Error, INFO == Information, WARN == Warning
** id -> name of function, or other identifier
*/
static char *fvwm_msg_strings[] =
{ "<<DEBUG>>", "", "", "<<WARNING>>", "<<ERROR>>" };
void fvwm_msg(fvwm_msg_type type, char *id, char *msg, ...)
{
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
  sprintf(buffer, "%.2d:%.2d:%.2d %6ld",
          t_ptr->tm_hour, t_ptr->tm_min, t_ptr->tm_sec, time_taken);
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
	    fvwm_msg_strings[(int)type]);
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
	    fvwm_msg_strings[(int)type]);
  }

  if (type == ECHO)
  {
    /* user echos must be printed as a literal string */
    fprintf(stderr, "%s", msg);
  }
  else
  {
    va_start(args,msg);
    vfprintf(stderr, msg, args);
    va_end(args);
  }
  fprintf(stderr,"\n");

  if (type == ERR)
  {
    /* I hate to use a fixed length but this will do for now */
    char tmp[2 * MAX_TOKEN_LENGTH];
    sprintf(tmp,"[FVWM][%s]: %s ",id, fvwm_msg_strings[(int)type]);
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


/****************************************************************************
 *
 * For menus, move, and resize operations, we can effect keyboard
 * shortcuts by warping the pointer.
 *
 ****************************************************************************/
void Keyboard_shortcuts(
  XEvent *Event, FvwmWindow *fw, int *x_defect, int *y_defect, int ReturnEvent)
{
  int x_move_size = 0;
  int y_move_size = 0;

  if (fw)
  {
    x_move_size = fw->hints.width_inc;
    y_move_size = fw->hints.height_inc;
  }
  fvwmlib_keyboard_shortcuts(
    dpy, Scr.screen, Event, x_move_size, y_move_size, x_defect, y_defect,
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

Bool intersect_xrectangles(XRectangle *r1, XRectangle *r2)
{
  int x1 = max(r1->x, r2->x);
  int y1 = max(r1->y, r2->y);
  int x2 = min(r1->x + r1->width, r2->x + r2->width);
  int y2 = min(r1->y + r1->height, r2->y + r2->height);

  r1->x = x1;
  r1->y = y1;
  r1->width = x2 - x1;
  r1->height = y2 - y1;

  if (x2 > x1 && y2 > y1)
    return True;
  else
    return False;
}
