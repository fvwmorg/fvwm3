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

/*
** fvwmlib_get_target_window and fvwmlib_keyboard_shortcuts - handle window
** selection from modules and fvwm.
*/

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <X11/keysym.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>

#include "fvwmlib.h"

void fvwmlib_keyboard_shortcuts(
    Display *dpy, int screen, XEvent *Event, int x_move_size, int y_move_size,
    int *x_defect, int *y_defect, int ReturnEvent)
{
  int x;
  int y;
  int x_root;
  int y_root;
  int x_move;
  int y_move;
  KeySym keysym;
  Window JunkRoot;
  unsigned int JunkMask;

  if (y_move_size < DEFAULT_KDB_SHORTCUT_MOVE_DISTANCE)
    y_move_size = DEFAULT_KDB_SHORTCUT_MOVE_DISTANCE;
  if (x_move_size < DEFAULT_KDB_SHORTCUT_MOVE_DISTANCE)
    x_move_size = DEFAULT_KDB_SHORTCUT_MOVE_DISTANCE;
  if (Event->xkey.state & ControlMask)
    x_move_size = y_move_size = KDB_SHORTCUT_MOVE_DISTANCE_SMALL;
  if (Event->xkey.state & ShiftMask)
    x_move_size = y_move_size = KDB_SHORTCUT_MOVE_DISTANCE_BIG;

  keysym = XLookupKeysym(&Event->xkey,0);

  x_move = 0;
  y_move = 0;
  switch(keysym)
  {
  case XK_Up:
  case XK_KP_8:
  case XK_k:
  case XK_p:
    y_move = -y_move_size;
    break;
  case XK_Down:
  case XK_KP_2:
  case XK_n:
  case XK_j:
    y_move = y_move_size;
    break;
  case XK_Left:
  case XK_KP_4:
  case XK_b:
  case XK_h:
    x_move = -x_move_size;
    break;
  case XK_Right:
  case XK_KP_6:
  case XK_f:
  case XK_l:
    x_move = x_move_size;
    break;
  case XK_KP_1:
    x_move = -x_move_size;
    y_move = y_move_size;
    break;
  case XK_KP_3:
    x_move = x_move_size;
    y_move = y_move_size;
    break;
  case XK_KP_7:
    x_move = -x_move_size;
    y_move = -y_move_size;
    break;
  case XK_KP_9:
    x_move = x_move_size;
    y_move = -y_move_size;
    break;
  case XK_Return:
  case XK_KP_Enter:
  case XK_space:
    /* beat up the event */
    Event->type = ReturnEvent;
    break;
  case XK_Escape:
    /* simple code to bag out of move - CKH */
    /* return keypress event instead */
    Event->type = KeyPress;
    Event->xkey.keycode = XKeysymToKeycode(Event->xkey.display,keysym);
    break;
  default:
    break;
  }
  if (x_move || y_move)
  {
    int x_def_new = 0;
    int y_def_new = 0;

    XQueryPointer(dpy, RootWindow(dpy, screen), &JunkRoot, &Event->xany.window,
		  &x_root, &y_root, &x, &y, &JunkMask);
    if (x + x_move < 0)
    {
      x_def_new = x + x_move;
      x_move = -x;
    }
    else if (x + x_move >= DisplayWidth(dpy, DefaultScreen(dpy)))
    {
      x_def_new = x + x_move - DisplayWidth(dpy, DefaultScreen(dpy));
      x_move = DisplayWidth(dpy, DefaultScreen(dpy)) - x - 1;
    }
    if (y + y_move < 0)
    {
      y_def_new = y + y_move;
      y_move = -y;
    }
    else if (y + y_move >= DisplayHeight(dpy, DefaultScreen(dpy)))
    {
      y_def_new = y + y_move - DisplayHeight(dpy, DefaultScreen(dpy));
      y_move = DisplayHeight(dpy, DefaultScreen(dpy)) - y - 1;
    }
    if (x_defect)
    {
      int diff = 0;

      *x_defect += x_def_new;
      if (*x_defect > 0 && x_move < 0)
      {
	diff = min(*x_defect, -x_move);
      }
      else if (*x_defect < 0 && x_move > 0)
      {
	diff = max(*x_defect, -x_move);
      }
      *x_defect -= diff;
      x_move += diff;
    }
    if (y_defect)
    {
      int diff = 0;

      *y_defect += y_def_new;
      if (*y_defect > 0 && y_move < 0)
      {
	diff = min(*y_defect, -y_move);
      }
      else if (*y_defect < 0 && y_move > 0)
      {
	diff = max(*y_defect, -y_move);
      }
      *y_defect -= diff;
      y_move += diff;
    }
    if (x_move || y_move)
    {
      XWarpPointer(dpy, None, RootWindow(dpy, screen), 0, 0, 0, 0,
		   x_root + x_move, y_root + y_move);
    }
    /* beat up the event */
    Event->type = MotionNotify;
    Event->xkey.x += x_move;
    Event->xkey.y += y_move;
    Event->xkey.x_root += x_move;
    Event->xkey.y_root += y_move;
  }
}

void fvwmlib_get_target_window(
    Display *dpy, int screen, char *MyName, Window *app_win,
    Bool return_subwindow)
{
  XEvent eventp;
  int val = -10,trials;
  Bool finished = False;
  Bool canceled = False;
  Window Root = RootWindow(dpy, screen);
  int is_key_pressed = 0;
  int is_button_pressed = 0;
  KeySym keysym;

  trials = 0;
  while((trials <10)&&(val != GrabSuccess))
  {
    val=XGrabPointer(dpy, Root, True,
		     ButtonPressMask | ButtonReleaseMask,
		     GrabModeAsync, GrabModeAsync, Root,
		     XCreateFontCursor(dpy,XC_crosshair),
		     CurrentTime);
    switch (val)
    {
    case GrabInvalidTime:
    case GrabNotViewable:
      /* give up */
      trials += 100000;
      break;
    case GrabSuccess:
      break;
    case AlreadyGrabbed:
    case GrabFrozen:
    default:
      usleep(10000);
      trials++;
      break;
    }
  }
  if(val != GrabSuccess)
  {
    fprintf(stderr,"%s: Couldn't grab the cursor!\n",MyName);
    exit(1);
  }
  MyXGrabKeyboard(dpy);

  while (!finished && !canceled)
  {
    XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
                   KeyPressMask | KeyReleaseMask, &eventp);
    switch (eventp.type)
    {
    case KeyPress:
      is_key_pressed++;
      break;
    case KeyRelease:
      keysym = XLookupKeysym(&eventp.xkey,0);
      if( !is_key_pressed ) break;
      switch (keysym)
      {
      case XK_Escape:
        canceled = True;
        break;
      case XK_space:
      case XK_Return:
      case XK_KP_Enter:
        finished = True;
        break;
      default:
        fvwmlib_keyboard_shortcuts(dpy, screen, &eventp, 0, 0, NULL, NULL, 0);
        break;
      }
      break;
    case ButtonPress:
      is_button_pressed++;
      break;
    case ButtonRelease:
      if( is_button_pressed ) finished = True;
      break;
    }
  }

  MyXUngrabKeyboard(dpy);
  XUngrabPointer(dpy, CurrentTime);
  XSync(dpy,0);
  if (canceled)
  {
    *app_win = None;
    return;
  }
  *app_win = eventp.xany.window;
  if(return_subwindow && eventp.xbutton.subwindow != None)
    *app_win = eventp.xbutton.subwindow;

  return;
}

Window fvwmlib_client_window(Display *dpy, Window input)
{
  Atom _XA_WM_STATE;
  unsigned int nchildren;
  Window root, parent, *children,target;
  unsigned long nitems, bytesafter;
  unsigned char *prop;
  Atom atype;
  int aformat;
  int i;

  _XA_WM_STATE = XInternAtom (dpy, "WM_STATE", False);

  if (XGetWindowProperty(dpy, input, _XA_WM_STATE , 0L, 3L , False,
			 _XA_WM_STATE, &atype, &aformat, &nitems, &bytesafter,
			 &prop) == Success) {
    if(prop != NULL) {
      XFree(prop);
      return input;
    }
  }

  if (!XQueryTree(dpy, input, &root, &parent, &children, &nchildren))
    return None;

  for (i = 0; i < nchildren; i++) {
    target = fvwmlib_client_window(dpy, children[i]);
    if(target != None) {
      XFree((char *)children);
      return target;
    }
  }
  XFree((char *)children);
  return None;
}
