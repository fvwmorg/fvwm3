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

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <X11/keysym.h>

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "externs.h"
#include "libs/Colorset.h"
#include "bindings.h"
#include "misc.h"
#include "cursor.h"
#include "functions.h"
#include "screen.h"
#include "defaults.h"
#include "builtins.h"
#include "module_interface.h"
#include "borders.h"
#include "geometry.h"
#include "events.h"
#include "gnome.h"
#include "icons.h"
#include "menus.h"
#include "virtual.h"
#include "decorations.h"
#include "add_window.h"
#include "update.h"
#include "style.h"
#include "focus.h"
#ifdef HAVE_STROKE
#include "stroke.h"
#endif /* HAVE_STROKE */

static char *ReadTitleButton(
    char *s, TitleButton *tb, Boolean append, int button);

extern float rgpctMovementDefault[32];
extern int cpctMovementDefault;
extern int cmsDelayDefault;

char *ModulePath = FVWM_MODULEDIR;
int moduleTimeout = DEFAULT_MODULE_TIMEOUT;
static char *exec_shell_name="/bin/sh";
/* button state strings must match the enumerated states */
static char  *button_states[MaxButtonState] =
{
  "ActiveUp",
  "ActiveDown",
  "Inactive",
  "ToggledActiveUp",
  "ToggledActiveDown",
  "ToggledInactive",
};





/***********************************************************************
 *
 *  WindowShade -- shades or unshades a window (veliaa@rpi.edu)
 *
 *  Args: 1 -- shade, 2 -- unshade  No Arg: toggle
 *
 ***********************************************************************
 *
 *  Animation added.
 *     Based on code from AfterStep-1.0 (CT: ctibirna@gch.ulaval.ca)
 *     Additional fixes by Tomas Ogren <stric@ing.umu.se>
 *
 *  Builtin function: WindowShade
 ***********************************************************************/
void WindowShade(F_CMD_ARGS)
{
  int bwl;
  int bwr;
  int bw;
  int bht;
  int bhb;
  int bh;
  int cw;
  int ch;
  int step = 1;
  int toggle;
  int grav = NorthWestGravity;
  int client_grav = NorthWestGravity;
  int move_parent_too = False;
  rectangle frame_g;
  rectangle parent_g;
  rectangle diff;
  rectangle pdiff;
  rectangle big_g;
  rectangle small_g;
  Bool do_scroll;

  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
    return;
  if (tmp_win == NULL || IS_ICONIFIED(tmp_win))
    return;

  /* parse arguments */
  toggle = ParseToggleArgument(action, NULL, -1, 0);
  if (toggle == -1)
  {
    if (GetIntegerArguments(action, NULL, &toggle, 1) > 0)
    {
      if (toggle == 1)
        toggle = 1;
      else if (toggle == 2)
        toggle = 0;
      else
        toggle = -1;
    }
  }
  if (toggle == -1)
    toggle = (IS_SHADED(tmp_win)) ? 0 : 1;

  /* prepare some convenience variables */
  frame_g = tmp_win->frame_g;
  big_g = (IS_MAXIMIZED(tmp_win)) ? tmp_win->max_g : tmp_win->normal_g;
  get_relative_geometry(&big_g, &big_g);
  bwl = tmp_win->boundary_width;
  bwr = tmp_win->boundary_width;
  bw = bwl + bwr;
  bht = tmp_win->boundary_width;
  bhb = tmp_win->boundary_width;
  if (HAS_BOTTOM_TITLE(tmp_win))
    bhb += tmp_win->title_g.height;
  else
    bht += tmp_win->title_g.height;
  bh = bht + bhb;
  cw = big_g.width - bw;
  ch = big_g.height - bh;
  do_scroll = !DO_SHRINK_WINDOWSHADE(tmp_win);
  /* calcuate the step size */
  if (tmp_win->shade_anim_steps > 0)
    step = big_g.height / tmp_win->shade_anim_steps;
  else if (tmp_win->shade_anim_steps < 0) /* if it's -ve it means pixels */
    step = -tmp_win->shade_anim_steps;
  if (step <= 0)  /* We don't want an endless loop, do we? */
    step = 1;
  if (tmp_win->shade_anim_steps)
  {
    grav = (HAS_BOTTOM_TITLE(tmp_win)) ? SouthEastGravity : NorthWestGravity;
    if (!do_scroll)
      client_grav = grav;
    else
      client_grav =
	(HAS_BOTTOM_TITLE(tmp_win)) ? NorthWestGravity : SouthEastGravity;
    set_decor_gravity(tmp_win, grav, grav, client_grav);
    move_parent_too = HAS_BOTTOM_TITLE(tmp_win);
  }

  if (IS_SHADED(tmp_win) && toggle == 0)
  {
    /* unshade window */
    SET_SHADED(tmp_win, 0);

    if (tmp_win->shade_anim_steps != 0)
    {
      XMoveResizeWindow(dpy, tmp_win->Parent, bwl, bht, cw, 1);
      XMoveWindow(dpy, tmp_win->w, 0,
		  (client_grav == SouthEastGravity) ? -ch + 1 : 0);
      XLowerWindow(dpy, tmp_win->decor_w);
      parent_g.x = bwl;
      parent_g.y = bht + (HAS_BOTTOM_TITLE(tmp_win) ? -step : 0);
      parent_g.width = cw;
      parent_g.height = 0;
      pdiff.x = 0;
      pdiff.y = 0;
      pdiff.width = 0;
      pdiff.height = step;
      diff.x = 0;
      diff.y = HAS_BOTTOM_TITLE(tmp_win) ? -step : 0;
      diff.width = 0;
      diff.height = step;

      /* This causes a ConfigureNotify if the client size has changed while
       * the window was shaded. */
      XResizeWindow(dpy, tmp_win->w, cw, ch);
      /* make the decor window the full sze before the animation unveils it */
      XMoveResizeWindow(dpy, tmp_win->decor_w, 0, HAS_BOTTOM_TITLE(tmp_win)
        ? frame_g.height - big_g.height : 0, big_g.width, big_g.height);
      /* draw the border decoration iff backing store is on */
      if (Scr.use_backing_store != NotUseful)
      { /* eek! fvwm doesn't know the backing_store setting for windows */
        XWindowAttributes xwa;

        if (XGetWindowAttributes(dpy, tmp_win->decor_w, &xwa)
            && xwa.backing_store != NotUseful)
        {
          tmp_win->frame_g.width = big_g.width;
          tmp_win->frame_g.height = big_g.height;
          DrawDecorations(
	    tmp_win, DRAW_FRAME, Scr.Focus == tmp_win, True, None);
        }
      }
      while (frame_g.height + diff.height < big_g.height)
      {
	parent_g.x += pdiff.x;
	parent_g.y += pdiff.y;
	parent_g.width += pdiff.width;
	parent_g.height += pdiff.height;
	frame_g.x += diff.x;
	frame_g.y += diff.y;
	frame_g.width += diff.width;
	frame_g.height += diff.height;
	if (move_parent_too)
	  XMoveResizeWindow(
	    dpy, tmp_win->Parent, parent_g.x, parent_g.y, parent_g.width,
	    parent_g.height);
	else
	  XResizeWindow(dpy, tmp_win->Parent, parent_g.width, parent_g.height);
	XMoveResizeWindow(
	  dpy, tmp_win->frame, frame_g.x, frame_g.y, frame_g.width,
	  frame_g.height);
        FlushAllMessageQueues();
        XSync(dpy, 0);
      }
      if (client_grav == SouthEastGravity && move_parent_too)
      {
	/* We must finish above loop to the very end for this special case.
	 * Otherwise there is a visible jump of the client window. */
	XMoveResizeWindow(
	  dpy, tmp_win->Parent, parent_g.x,
	  bht - (big_g.height - frame_g.height), parent_g.width, ch);
	XMoveResizeWindow(
	  dpy, tmp_win->frame, big_g.x, big_g.y, big_g.width, big_g.height);
      }
      else
      {
	XMoveWindow(dpy, tmp_win->w, 0, 0);
      }
    }
    else
    {
      XMoveResizeWindow(dpy, tmp_win->w, 0, 0, cw, ch);
      if (HAS_BOTTOM_TITLE(tmp_win))
      {
	XMoveResizeWindow(dpy, tmp_win->Parent, bwl, bht - ch + 1, cw, ch);
	set_decor_gravity(
	  tmp_win, SouthEastGravity, SouthEastGravity, NorthWestGravity);
      }
      else
      {
	XMoveResizeWindow(dpy, tmp_win->Parent, bwl, bht, cw, ch);
	set_decor_gravity(
	  tmp_win, NorthWestGravity, NorthWestGravity, NorthWestGravity);
      }
      XLowerWindow(dpy, tmp_win->decor_w);
      XMoveResizeWindow(
	dpy, tmp_win->frame, big_g.x, big_g.y, big_g.width, big_g.height);
    }
    set_decor_gravity(
      tmp_win, NorthWestGravity, NorthWestGravity, NorthWestGravity);
    tmp_win->frame_g.height = big_g.height + 1;
    SetupFrame(
      tmp_win, big_g.x, big_g.y, big_g.width, big_g.height, True);
    tmp_win->frame_g = big_g;
    update_absolute_geometry(tmp_win);
    BroadcastConfig(M_CONFIGURE_WINDOW, tmp_win);
    BroadcastPacket(M_DEWINDOWSHADE, 3, tmp_win->w, tmp_win->frame,
                    (unsigned long)tmp_win);
  }
  else if (!IS_SHADED(tmp_win) && toggle == 1)
  {
    /* shade window */
    SET_SHADED(tmp_win, 1);
    get_shaded_geometry(tmp_win, &small_g, &big_g);
    if (tmp_win->shade_anim_steps != 0)
    {
      parent_g.x = bwl;
      parent_g.y = bht;
      parent_g.width = cw;
      parent_g.height = frame_g.height - bh;
      pdiff.x = 0;
      pdiff.y = 0;
      pdiff.width = 0;
      pdiff.height = -step;
      diff.x = 0;
      diff.y = HAS_BOTTOM_TITLE(tmp_win) ? step : 0;
      diff.width = 0;
      diff.height = -step;

      while (frame_g.height + diff.height > small_g.height)
      {
	parent_g.x += pdiff.x;
	parent_g.y += pdiff.y;
	parent_g.width += pdiff.width;
	parent_g.height += pdiff.height;
	frame_g.x += diff.x;
	frame_g.y += diff.y;
	frame_g.width += diff.width;
	frame_g.height += diff.height;
	if (move_parent_too)
	{
	  XMoveResizeWindow(
	    dpy, tmp_win->frame, frame_g.x, frame_g.y, frame_g.width,
	    frame_g.height);
	  XMoveResizeWindow(
	    dpy, tmp_win->Parent, parent_g.x, parent_g.y, parent_g.width,
	    parent_g.height);
	}
	else
	{
	  XResizeWindow(dpy, tmp_win->Parent, parent_g.width, parent_g.height);
	  XMoveResizeWindow(
	    dpy, tmp_win->frame, frame_g.x, frame_g.y, frame_g.width,
	    frame_g.height);
	}
        FlushAllMessageQueues();
        XSync(dpy, 0);
      }
    }
    /* All this stuff is necessary to prevent flickering */
    set_decor_gravity(tmp_win, grav, UnmapGravity, client_grav);
    XMoveResizeWindow(
      dpy, tmp_win->frame, small_g.x, small_g.y, small_g.width,
      small_g.height);
    XResizeWindow(dpy, tmp_win->Parent, cw, 1);
    XMoveWindow(dpy, tmp_win->decor_w, 0, 0);
    XRaiseWindow(dpy, tmp_win->decor_w);
    XMapWindow(dpy, tmp_win->Parent);
    /* domivogt (28-Dec-1999): For some reason the XMoveResize() on the frame
     * window removes the input focus from the client window.  I have no idea
     * why, but if we explicitly restore the focus here everything works fine.
     */
    if (Scr.Focus == tmp_win)
      FOCUS_SET(tmp_win->w);
    set_decor_gravity(
      tmp_win, NorthWestGravity, NorthWestGravity, NorthWestGravity);
    /* Finally let SetupFrame take care of the window */
    SetupFrame(
      tmp_win, small_g.x, small_g.y, small_g.width, small_g.height, False);
    tmp_win->frame_g = small_g;
    update_absolute_geometry(tmp_win);
    BroadcastConfig(M_CONFIGURE_WINDOW, tmp_win);
    BroadcastPacket(
      M_WINDOWSHADE, 3, tmp_win->w, tmp_win->frame, (unsigned long)tmp_win);
  }
  DrawDecorations(
    tmp_win, DRAW_FRAME | DRAW_BUTTONS, (Scr.Hilite == tmp_win), True, None);
  FlushAllMessageQueues();
  XSync(dpy, 0);

  GNOME_SetHints(tmp_win);
  GNOME_SetWinArea(tmp_win);
}

void Bell(F_CMD_ARGS)
{
  XBell(dpy, 0);
}

void add_another_item(F_CMD_ARGS)
{
  if (Scr.last_added_item.type == ADDED_MENU)
    add_another_menu_item(action);
  else if (Scr.last_added_item.type == ADDED_FUNCTION)
    AddToFunction(Scr.last_added_item.item, action);
#ifdef USEDECOR
  else if (Scr.last_added_item.type == ADDED_DECOR) {
    FvwmDecor *tmp = &Scr.DefaultDecor;
    for (; tmp; tmp = tmp->next)
      if (tmp == Scr.last_added_item.item)
	break;
    if (!tmp)
      return;
    AddToDecor(tmp, action);
  }
#endif /* USEDECOR */
}

void destroy_fvwmfunc(F_CMD_ARGS)
{
  FvwmFunction *func;
  char *token;

  GetNextToken(action,&token);
  if (!token)
    return;
  func = FindFunction(token);
  free(token);
  if (!func)
    return;
  if (Scr.last_added_item.type == ADDED_FUNCTION)
    set_last_added_item(ADDED_NONE, NULL);
  DestroyFunction(func);

  return;
}

void add_item_to_func(F_CMD_ARGS)
{
  FvwmFunction *func;
  char *token;

  action = GetNextToken(action,&token);
  if (!token)
    return;
  func = FindFunction(token);
  if(func == NULL)
    func = NewFvwmFunction(token);

  /* Set + state to last function */
  set_last_added_item(ADDED_FUNCTION, func);

  free(token);
  AddToFunction(func, action);

  return;
}


void Nop_func(F_CMD_ARGS)
{

}


void movecursor(F_CMD_ARGS)
{
  int x = 0, y = 0;
  int val1, val2, val1_unit, val2_unit;
  int virtual_x, virtual_y;
  int pan_x, pan_y;
  int x_pages, y_pages;

  if (GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit) != 2)
  {
    fvwm_msg(ERR, "movecursor", "CursorMove needs 2 arguments");
    return;
  }

  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
                 &x, &y, &JunkX, &JunkY, &JunkMask);

  x = x + val1 * val1_unit / 100;
  y = y + val2 * val2_unit / 100;

  virtual_x = Scr.Vx;
  virtual_y = Scr.Vy;
  if (x >= 0)
    x_pages = x / Scr.MyDisplayWidth;
  else
    x_pages = ((x + 1) / Scr.MyDisplayWidth) - 1;
  virtual_x += x_pages * Scr.MyDisplayWidth;
  x -= x_pages * Scr.MyDisplayWidth;
  if (virtual_x < 0)
  {
    x += virtual_x;
    virtual_x = 0;
  }
  else if (virtual_x > Scr.VxMax)
  {
    x += virtual_x - Scr.VxMax;
    virtual_x = Scr.VxMax;
  }

  if (y >= 0)
    y_pages = y / Scr.MyDisplayHeight;
  else
    y_pages = ((y + 1) / Scr.MyDisplayHeight) - 1;
  virtual_y += y_pages * Scr.MyDisplayHeight;
  y -= y_pages * Scr.MyDisplayHeight;
  if (virtual_y < 0)
  {
    y += virtual_y;
    virtual_y = 0;
  }
  else if (virtual_y > Scr.VyMax)
  {
    y += virtual_y - Scr.VyMax;
    virtual_y = Scr.VyMax;
  }
  if (virtual_x != Scr.Vx || virtual_y != Scr.Vy)
    MoveViewport(virtual_x, virtual_y, True);
  pan_x = (Scr.EdgeScrollX != 0) ? 2 : 0;
  pan_y = (Scr.EdgeScrollY != 0) ? 2 : 0;
  /* prevent paging if EdgeScroll is active */
  if (x >= Scr.MyDisplayWidth - pan_x)
    x = Scr.MyDisplayWidth - pan_x -1;
  else if (x < pan_x)
    x = pan_x;
  if (y >= Scr.MyDisplayHeight - pan_y)
    y = Scr.MyDisplayHeight - pan_y - 1;
  else if (y < pan_y)
    y = pan_y;

  XWarpPointer(dpy, None, Scr.Root, 0, 0, Scr.MyDisplayWidth,
	       Scr.MyDisplayHeight, x, y);
  return;
}


void destroy_function(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_DESTROY, ButtonRelease))
    return;

  if(check_if_function_allowed(F_DESTROY,tmp_win,True,NULL) == 0)
  {
    XBell(dpy, 0);
    return;
  }

  if (XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
		   &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
    destroy_window(tmp_win);
  else
    XKillClient(dpy, tmp_win->w);
  XSync(dpy,0);
}

void delete_function(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_DESTROY,ButtonRelease))
    return;

  if(check_if_function_allowed(F_DELETE,tmp_win,True,NULL) == 0)
  {
    XBell(dpy, 0);
    return;
  }

  if (WM_DELETES_WINDOW(tmp_win))
  {
    send_clientmessage (dpy, tmp_win->w, _XA_WM_DELETE_WINDOW, CurrentTime);
    return;
  }
  else
    XBell (dpy, 0);
  XSync(dpy,0);
}

void close_function(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_DESTROY,ButtonRelease))
    return;

  if(check_if_function_allowed(F_CLOSE,tmp_win,True,NULL) == 0)
  {
    XBell(dpy, 0);
    return;
  }

  if (WM_DELETES_WINDOW(tmp_win))
  {
    send_clientmessage (dpy, tmp_win->w, _XA_WM_DELETE_WINDOW, CurrentTime);
    return;
  }
  else if (XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
			&JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
  {
    destroy_window(tmp_win);
  }
  else
  {
    XKillClient(dpy, tmp_win->w);
  }
  XSync(dpy,0);
}

void restart_function(F_CMD_ARGS)
{
  Done(1, action);
}

void exec_setup(F_CMD_ARGS)
{
  char *arg=NULL;
  static char shell_set = 0;

  if (shell_set)
    free(exec_shell_name);
  shell_set = 1;
  action = GetNextToken(action,&arg);
  if (arg) /* specific shell was specified */
  {
    exec_shell_name = arg;
  }
  else /* no arg, so use $SHELL -- not working??? */
  {
    if (getenv("SHELL"))
      exec_shell_name = strdup(getenv("SHELL"));
    else
      /* if $SHELL not set, use default */
      exec_shell_name = strdup("/bin/sh");
  }
}

void exec_function(F_CMD_ARGS)
{
  char *cmd=NULL;

  /* if it doesn't already have an 'exec' as the first word, add that
   * to keep down number of procs started */
  /* need to parse string better to do this right though, so not doing this
     for now... */
#if 0
  if (strncasecmp(action,"exec",4)!=0)
  {
    cmd = (char *)safemalloc(strlen(action)+6);
    strcpy(cmd,"exec ");
    strcat(cmd,action);
  }
  else
#endif
  {
    cmd = strdup(action);
  }
  if (!cmd)
    return;
  /* Use to grab the pointer here, but the fork guarantees that
   * we wont be held up waiting for the function to finish,
   * so the pointer-gram just caused needless delay and flashing
   * on the screen */
  /* Thought I'd try vfork and _exit() instead of regular fork().
   * The man page says that its better. */
  /* Not everyone has vfork! */
  if (!(fork())) /* child process */
  {
    if (execl(exec_shell_name, exec_shell_name, "-c", cmd, NULL)==-1)
    {
      fvwm_msg(ERR,"exec_function","execl failed (%s)",strerror(errno));
      exit(100);
    }
  }
  free(cmd);
  return;
}

static void refresh_window(Window w)
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;

  valuemask = CWOverrideRedirect | CWBackingStore | CWSaveUnder | CWBackPixmap;
  attributes.override_redirect = True;
  attributes.save_under = False;
  attributes.background_pixmap = None;
  attributes.backing_store = NotUseful;
  w = XCreateWindow(dpy,
		    w,
		    0, 0,
		    (unsigned int) Scr.MyDisplayWidth,
		    (unsigned int) Scr.MyDisplayHeight,
		    (unsigned int) 0,
		    CopyFromParent, (unsigned int) CopyFromParent,
		    CopyFromParent, valuemask,
		    &attributes);
  XMapWindow(dpy, w);
  if (Scr.flags.do_need_window_update)
  {
    flush_window_updates();
  }
  XDestroyWindow(dpy, w);
  XFlush(dpy);
}

void refresh_function(F_CMD_ARGS)
{
  refresh_window(Scr.Root);
}

void refresh_win_function(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context,CRS_SELECT,ButtonRelease))
    return;
  refresh_window((context == C_ICON)? tmp_win->icon_w : tmp_win->frame);
}


void wait_func(F_CMD_ARGS)
{
  Bool done = False;
  Bool redefine_cursor = False;
  char *escape;
  Window nonewin = None;
  extern FvwmWindow *Tmp_win;
  char *wait_string, *rest;

  /* try to get a single token */
  rest = GetNextToken(action, &wait_string);
  if (wait_string)
  {
    while (*rest && isspace((unsigned char)*rest))
      rest++;
    if (*rest)
    {
      int i;
      char *temp;

      /* nope, multiple tokens - try old syntax */

      /* strip leading and trailing whitespace */
      temp = action;
      while (*temp && isspace((unsigned char)*temp))
        temp++;
      wait_string = strdup(temp);
      for (i = strlen(wait_string) - 1; i >= 0 && isspace(wait_string[i]); i--)
      {
        wait_string[i] = 0;
      }
    }
  }
  else
  {
    wait_string = strdup("");
  }

  while(!done)
  {
    if (BUSY_WAIT & Scr.BusyCursor)
    {
      XDefineCursor(dpy, Scr.Root, Scr.FvwmCursors[CRS_WAIT]);
      redefine_cursor = True;
    }
    if(My_XNextEvent(dpy, &Event))
    {
      DispatchEvent(False);
      if(Event.type == MapNotify)
      {
        if (!*wait_string)
          done = True;
        if((Tmp_win)&&(matchWildcards(wait_string, Tmp_win->name)==True))
          done = True;
        else if((Tmp_win)&&(Tmp_win->class.res_class)&&
           (matchWildcards(wait_string, Tmp_win->class.res_class)==True))
          done = True;
        else if((Tmp_win)&&(Tmp_win->class.res_name)&&
           (matchWildcards(wait_string, Tmp_win->class.res_name)==True))
          done = True;
      }
      else if (Event.type == KeyPress)
      {
        escape = CheckBinding(Scr.AllBindings,
                              STROKE_ARG(0)
                              Event.xkey.keycode,
                              Event.xkey.state, GetUnusedModifiers(),
                              GetContext(Tmp_win,&Event,&nonewin),
                              KEY_BINDING);
        if (escape != NULL)
        {
	  if (!strcasecmp(escape,"escapefunc"))
            done = True;
        }
      }
    }
  }
  if (redefine_cursor)
    XDefineCursor(dpy, Scr.Root, Scr.FvwmCursors[CRS_ROOT]);

  free(wait_string);
}

void quit_func(F_CMD_ARGS)
{
  if (master_pid != getpid())
    kill(master_pid, SIGTERM);
  Done(0,NULL);
}

void quit_screen_func(F_CMD_ARGS)
{
  Done(0,NULL);
}

void echo_func(F_CMD_ARGS)
{
  unsigned int len;

  if (!action)
    action = "";
  len = strlen(action);
  if (len != 0)
  {
    if (action[len-1]=='\n')
      action[len-1]='\0';
  }
  fvwm_msg(INFO,"Echo",action);
}

void SetColormapFocus(F_CMD_ARGS)
{
  if (MatchToken(action,"FollowsFocus"))
  {
    Scr.ColormapFocus = COLORMAP_FOLLOWS_FOCUS;
  }
  else if (MatchToken(action,"FollowsMouse"))
  {
    Scr.ColormapFocus = COLORMAP_FOLLOWS_MOUSE;
  }
  else
  {
    fvwm_msg(ERR,"SetColormapFocus",
             "ColormapFocus requires 1 arg: FollowsFocus or FollowsMouse");
    return;
  }
}

void SetClick(F_CMD_ARGS)
{
  int val;

  if(GetIntegerArguments(action, NULL, &val, 1) != 1)
  {
    Scr.ClickTime = DEFAULT_CLICKTIME;
  }
  else
  {
    Scr.ClickTime = (val < 0)? 0 : val;
  }

  /* Use a negative value during startup and change sign afterwards. This
   * speeds things up quite a bit. */
  if (fFvwmInStartup)
    Scr.ClickTime = -Scr.ClickTime;
}


void imagePath_function(F_CMD_ARGS)
{
    SetImagePath( action );
}

/** Prepend rather than replace the image path.
    Used for obsolete PixmapPath and IconPath **/
static void obsolete_imagepaths( const char* pre_path )
{
    char* tmp = stripcpy( pre_path );
    char* path = alloca( strlen( tmp ) + strlen( GetImagePath() ) + 2 );

    strcpy( path, tmp );
    free( tmp );

    strcat( path, ":" );
    strcat( path, GetImagePath() );

    SetImagePath( path );
}


void iconPath_function(F_CMD_ARGS)
{
    fvwm_msg(ERR, "iconPath_function",
	     "IconPath is deprecated since 2.3.0; use ImagePath instead." );
    obsolete_imagepaths( action );
}

void pixmapPath_function(F_CMD_ARGS)
{
    fvwm_msg(ERR, "pixmapPath_function",
	     "PixmapPath is deprecated since 2.3.0; use ImagePath instead." );
    obsolete_imagepaths( action );
}


void setModulePath(F_CMD_ARGS)
{
    static int need_to_free = 0;
    setPath( &ModulePath, action, need_to_free );
    need_to_free = 1;
}


void setModuleTimeout(F_CMD_ARGS)
{
  int timeout;

  moduleTimeout = DEFAULT_MODULE_TIMEOUT;
  if (
       (GetIntegerArguments(action, NULL, &timeout, 1) == 1) &&
       (timeout > 0)
     )
  {
    moduleTimeout = timeout;
  }
}


void SetHiColor(F_CMD_ARGS)
{
  char *fore;
  char *back;
#ifdef USEDECOR
  if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor)
  {
    fvwm_msg(
      ERR, "SetHiColor",
      "Decors do not support the HilightColor command anymore."
      " Please use 'Style <stylename> HilightFore <forecolor>' and"
      " 'Style <stylename> HilightBack <backcolor>' instead."
      " Sorry for the inconvenience.");
    return;
  }
#endif
  action = GetNextToken(action, &fore);
  GetNextToken(action, &back);
  if (fore && back)
  {
    action = safemalloc(strlen(fore) + strlen(back) + 29);
    sprintf(action, "* HilightFore %s, HilightBack %s", fore, back);
    ProcessNewStyle(F_PASS_ARGS);
  }
  if (fore)
    free(fore);
  if (back)
    free(back);
}


void SetHiColorset(F_CMD_ARGS)
{
  char *newaction;

#ifdef USEDECOR
  if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor)
  {
    fvwm_msg(
      ERR, "SetHiColorset",
      "Decors do not support the HilightColorset command anymore."
      " Please use 'Style <stylename> HilightColorset <colorset>' instead."
      " Sorry for the inconvenience.");
    return;
  }
#endif
  if (action)
  {
    newaction = safemalloc(strlen(action) + 32);
    sprintf(newaction, "* HilightColorset %s", action);
    action = newaction;
    ProcessNewStyle(F_PASS_ARGS);
  }
}


void do_title_style(F_CMD_ARGS, Bool do_add)
{

  char *parm;
  char *prev;
#ifdef USEDECOR
  FvwmDecor *decor = Scr.cur_decor ? Scr.cur_decor : &Scr.DefaultDecor;
#else
  FvwmDecor *decor = &Scr.DefaultDecor;
#endif

  Scr.flags.do_need_window_update = 1;
  decor->flags.has_changed = 1;
  decor->titlebar.flags.has_changed = 1;

  for (prev = action ; (parm = PeekToken(action, &action)); prev = action)
  {
    if (!do_add && StrEquals(parm,"centered"))
    {
      TB_JUSTIFICATION(decor->titlebar) = JUST_CENTER;
    }
    else if (!do_add && StrEquals(parm,"leftjustified"))
    {
      TB_JUSTIFICATION(decor->titlebar) = JUST_LEFT;
    }
    else if (!do_add && StrEquals(parm,"rightjustified"))
    {
      TB_JUSTIFICATION(decor->titlebar) = JUST_RIGHT;
    }
    else if (!do_add && StrEquals(parm,"height"))
    {
      int height = 0;
      int next = 0;

      if (!action || sscanf(action, "%d%n", &height, &next) <= 0 ||
	  height < MIN_FONT_HEIGHT || height > MAX_FONT_HEIGHT)
      {
	if (height != 0)
	{
	  fvwm_msg(ERR, "SetTitleStyle",
		   "bad height argument (height must be from 5 to 256)");
	  height = 0;
	}
      }
      if (decor->title_height != height)
      {
	decor->title_height = height;
	decor->flags.has_title_height_changed = 1;
      }
      if (action)
	action += next;
    }
    else
    {
      action = ReadTitleButton(prev, &decor->titlebar, do_add, -1);
    }
  }
}

void SetTitleStyle(F_CMD_ARGS)
{
  do_title_style(F_PASS_ARGS, False);
} /* SetTitleStyle */

#if defined(MULTISTYLE)
/*****************************************************************************
 *
 * Appends a titlestyle (veliaa@rpi.edu)
 *
 ****************************************************************************/
void AddTitleStyle(F_CMD_ARGS)
{
  do_title_style(F_PASS_ARGS, True);
}
#endif /* MULTISTYLE */

void ApplyDefaultFontAndColors(void)
{
  XGCValues gcv;
  unsigned long gcm;
  int wid;
  int hei;
  int cset = Scr.DefaultColorset;

  /* make GC's */
  gcm = GCFunction|GCFont|GCLineWidth|GCForeground|GCBackground;
  gcv.function = GXcopy;
  gcv.font = Scr.DefaultFont.font->fid;
  gcv.line_width = 0;
  if (cset >= 0) {
    gcv.foreground = Colorset[cset].fg;
    gcv.background = Colorset[cset].bg;
  } else {
    gcv.foreground = Scr.StdFore;
    gcv.background = Scr.StdBack;
  }
  if(Scr.StdGC)
    XChangeGC(dpy, Scr.StdGC, gcm, &gcv);
  else
    Scr.StdGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  gcm = GCFunction|GCLineWidth|GCForeground;
  if (cset >= 0)
    gcv.foreground = Colorset[cset].hilite;
  else
    gcv.foreground = Scr.StdHilite;
  if(Scr.StdReliefGC)
    XChangeGC(dpy, Scr.StdReliefGC, gcm, &gcv);
  else
    Scr.StdReliefGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  if (cset >= 0)
    gcv.foreground = Colorset[cset].shadow;
  else
    gcv.foreground = Scr.StdShadow;
  if(Scr.StdShadowGC)
    XChangeGC(dpy, Scr.StdShadowGC, gcm, &gcv);
  else
    Scr.StdShadowGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  /* update the geometry window for move/resize */
  if(Scr.SizeWindow != None)
  {
    Scr.SizeStringWidth =
      XTextWidth(Scr.DefaultFont.font, " +8888 x +8888 ", 15);
    wid = Scr.SizeStringWidth + 2 * SIZE_HINDENT;
    hei = Scr.DefaultFont.height + 2 * SIZE_VINDENT;
    if (cset >= 0)
    {
      SetWindowBackground(
	dpy, Scr.SizeWindow, wid, hei, &Colorset[cset], Pdepth, Scr.StdGC,
	False);
    }
    else
    {
      XSetWindowBackground(dpy, Scr.SizeWindow, Scr.StdBack);
    }

    if(Scr.gs.EmulateMWM)
    {
      XMoveResizeWindow(
	dpy, Scr.SizeWindow, (Scr.MyDisplayWidth - wid) / 2,
	(Scr.MyDisplayHeight - hei) / 2, wid, hei);
    }
    else
    {
      XMoveResizeWindow(dpy, Scr.SizeWindow, 0, 0, wid, hei);
    }
  }

  UpdateAllMenuStyles();
}

void HandleColorset(F_CMD_ARGS)
{
  int n = -1, ret;
  char *token;

  token = PeekToken(action, NULL);
  if (token == NULL)
    return;
  ret = sscanf(token, "%x", &n);

  if ((ret == 0) || (n < 0))
    return;

  LoadColorset(action);
  BroadcastColorset(n);

  if (n == Scr.DefaultColorset)
  {
    Scr.flags.do_need_window_update = 1;
    Scr.flags.has_default_color_changed = 1;
  }

  UpdateMenuColorset(n);
  update_style_colorset(n);
}


void SetDefaultIcon(F_CMD_ARGS)
{
  if (Scr.DefaultIcon)
    free(Scr.DefaultIcon);
  GetNextToken(action, &Scr.DefaultIcon);
}

void SetDefaultColorset(F_CMD_ARGS)
{
  int cset;

  if (GetIntegerArguments(action, NULL, &cset, 1) != 1)
    return;

  Scr.DefaultColorset = cset;
  if (Scr.DefaultColorset < 0)
    Scr.DefaultColorset = -1;
  AllocColorset(Scr.DefaultColorset);
  Scr.flags.do_need_window_update = 1;
  Scr.flags.has_default_color_changed = 1;
}

void SetDefaultColors(F_CMD_ARGS)
{
  char *fore = NULL;
  char *back = NULL;

  action = GetNextToken(action, &fore);
  action = GetNextToken(action, &back);
  if (!back)
    back = strdup("grey");
  if (!fore)
    fore = strdup("black");

  if (!StrEquals(fore, "-"))
  {
    XFreeColors(dpy, Pcmap, &Scr.StdFore, 1, 0);
    Scr.StdFore = GetColor(fore);
  }
  if (!StrEquals(back, "-"))
  {
    XFreeColors(dpy, Pcmap, &Scr.StdBack, 3, 0);
    Scr.StdBack = GetColor(back);
    Scr.StdHilite = GetHilite(Scr.StdBack);
    Scr.StdShadow = GetShadow(Scr.StdBack);
  }
  free(fore);
  free(back);

  Scr.DefaultColorset = -1;
  Scr.flags.do_need_window_update = 1;
  Scr.flags.has_default_color_changed = 1;
}

void LoadDefaultFont(F_CMD_ARGS)
{
  char *font;
  FvwmFont new_font;

  font = PeekToken(action, &action);
  if (!font)
  {
    /* Try 'fixed' */
    font = "";
  }

  if (!LoadFvwmFont(dpy, font, &new_font))
  {
    fvwm_msg(
      ERR, "SetDefaultFont", "Couldn't load font '%s' or 'fixed'\n", font);
    if (Scr.DefaultFont.font == NULL)
      exit(1);
    else
      return;
  }

  FreeFvwmFont(dpy, &Scr.DefaultFont);
  Scr.DefaultFont = new_font;
  /* set flags to indicate that the font has changed */
  Scr.flags.do_need_window_update = 1;
  Scr.flags.has_default_font_changed = 1;
}

void LoadIconFont(F_CMD_ARGS)
{
  char *newaction;

#ifdef USEDECOR
  if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor)
  {
    fvwm_msg(
      ERR, "LoadIconFont",
      "Decors do not support the IconFont command anymore."
      " Please use 'Style <stylename> IconFont <fontname>' instead."
      " Sorry for the inconvenience.");
    return;
  }
#endif
  if (action)
  {
    newaction = safemalloc(strlen(action) + 16);
    sprintf(newaction, "* IconFont %s", action);
    action = newaction;
    ProcessNewStyle(F_PASS_ARGS);
  }
}

void LoadWindowFont(F_CMD_ARGS)
{
  char *newaction;

#ifdef USEDECOR
  if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor)
  {
    fvwm_msg(
      ERR, "LoadWindowFont",
      "Decors do not support the WindowFont command anymore."
      " Please use 'Style <stylename> Font <fontname>' instead."
      " Sorry for the inconvenience.");
    return;
  }
#endif
  if (action)
  {
    newaction = safemalloc(strlen(action) + 16);
    sprintf(newaction, "* Font %s", action);
    action = newaction;
    ProcessNewStyle(F_PASS_ARGS);
  }
}

void FreeDecorFace(Display *dpy, DecorFace *df)
{
  switch (DFS_FACE_TYPE(df->style))
  {
  case GradientButton:
    /* - should we check visual is not TrueColor before doing this?

       XFreeColors(dpy, PictureCMap,
       df->u.grad.pixels, df->u.grad.npixels,
       AllPlanes); */
    free(df->u.grad.pixels);
    df->u.grad.pixels = NULL;
    break;

  case PixmapButton:
  case TiledPixmapButton:
    if (df->u.p)
      DestroyPicture(dpy, df->u.p);
    df->u.p = NULL;
    break;

  case VectorButton:
    if (df->u.vector.x)
    {
      free (df->u.vector.x);
      df->u.vector.x = NULL;
    }
    if (df->u.vector.y)
    {
      free (df->u.vector.y);
      df->u.vector.y = NULL;
    }
    break;

  default:
    break;
  }
#ifdef MULTISTYLE
  /* delete any compound styles */
  if (df->next)
  {
    FreeDecorFace(dpy, df->next);
    free(df->next);
  }
  df->next = NULL;
#endif
  memset(&df->style, 0, sizeof(df->style));
  DFS_FACE_TYPE(df->style) = SimpleButton;
}

/*****************************************************************************
 *
 * Reads a button face line into a structure (veliaa@rpi.edu)
 *
 ****************************************************************************/
Bool ReadDecorFace(char *s, DecorFace *df, int button, int verbose)
{
  int offset;
  char style[256], *file;
  char *action = s;

  /* some variants of scanf do not increase the assign count when %n is used,
   * so a return value of 1 is no error. */
  if (sscanf(s, "%256s%n", style, &offset) < 1)
  {
    if (verbose)
      fvwm_msg(ERR, "ReadDecorFace", "error in face `%s'", s);
    return False;
  }
  style[255] = 0;

  if (strncasecmp(style, "--", 2) != 0)
  {
    s += offset;

    FreeDecorFace(dpy, df);

    /* determine button style */
    if (strncasecmp(style,"Simple",6)==0)
    {
      memset(&df->style, 0, sizeof(df->style));
      DFS_FACE_TYPE(df->style) = SimpleButton;
    }
    else if (strncasecmp(style,"Default",7)==0)
    {
      int b = -1, n = sscanf(s, "%d%n", &b, &offset);

      if (n < 1)
      {
	if (button == -1)
	{
	  if(verbose)
	    fvwm_msg(ERR,"ReadDecorFace", "need default button number to load");
	  return False;
	}
	b = button;
      }
      else
      {
	b = BUTTON_INDEX(b);
	s += offset;
      }
      if (b >= 0 && b < NUMBER_OF_BUTTONS)
	LoadDefaultButton(df, b);
      else
      {
	if(verbose)
	  fvwm_msg(ERR,"ReadDecorFace", "button number out of range: %d", b);
	return False;
      }
    }
    else if (strncasecmp(style,"Vector",6)==0 ||
	     (strlen(style)<=2 && isdigit(*style)))
    {
      /* normal coordinate list button style */
      int i, num_coords, num, line_style;
      struct vector_coords *vc = &df->u.vector;

      /* get number of points */
      if (strncasecmp(style,"Vector",6)==0)
      {
	num = sscanf(s,"%d%n",&num_coords,&offset);
	s += offset;
      } else
	num = sscanf(style,"%d",&num_coords);

      if((num != 1)||(num_coords>32)||(num_coords<2))
      {
	if(verbose)fvwm_msg(ERR,"ReadDecorFace",
			    "Bad button style (2) in line: %s",action);
	return False;
      }

      vc->num = num_coords;
      vc->line_style = 0;
      vc->x = (int *) safemalloc (sizeof (int) * num_coords);
      vc->y = (int *) safemalloc (sizeof (int) * num_coords);

      /* get the points */
      for(i = 0; i < vc->num; ++i)
      {
	/* X x Y @ line_style */
	num = sscanf(s,"%dx%d@%d%n",&vc->x[i],&vc->y[i],
		     &line_style, &offset);
	if(num != 3)
	{
	  if(verbose)fvwm_msg(ERR,"ReadDecorFace",
			      "Bad button style (3) in line %s",
			      action);
	  return False;
	}
	if (line_style)
	{
	  vc->line_style |= (1 << i);
	}
	s += offset;
      }
      memset(&df->style, 0, sizeof(df->style));
      DFS_FACE_TYPE(df->style) = VectorButton;
    }
    else if (strncasecmp(style,"Solid",5)==0)
    {
      s = GetNextToken(s, &file);
      if (file)
      {
	memset(&df->style, 0, sizeof(df->style));
	DFS_FACE_TYPE(df->style) = SolidButton;
	df->u.back = GetColor(file);
	free(file);
      }
      else
      {
	if(verbose)
	  fvwm_msg(ERR,"ReadDecorFace",
		   "no color given for Solid face type: %s", action);
	return False;
      }
    }
    else if (strncasecmp(style+1, "Gradient", 8)==0)
    {
      char **s_colors;
      int npixels, nsegs, *perc;
      Pixel *pixels;

      if (!IsGradientTypeSupported(style[0]))
	return False;

      /* translate the gradient string into an array of colors etc */
      npixels = ParseGradient(s, &s, &s_colors, &perc, &nsegs);
      while (*s && isspace(*s))
	s++;
      if (npixels <= 0)
	return False;
      /* grab the colors */
      pixels = AllocAllGradientColors(s_colors, perc, nsegs, npixels);
      if (pixels == None)
	return False;

      df->u.grad.pixels = pixels;
      df->u.grad.npixels = npixels;
      memset(&df->style, 0, sizeof(df->style));
      DFS_FACE_TYPE(df->style) = GradientButton;
      df->u.grad.gradient_type = toupper(style[0]);
    }
    else if (strncasecmp(style,"Pixmap",6)==0
	     || strncasecmp(style,"TiledPixmap",11)==0)
    {
      s = GetNextToken(s, &file);
      df->u.p = CachePicture(dpy, Scr.NoFocusWin, NULL,
			     file,Scr.ColorLimit);
      if (df->u.p == NULL)
      {
	if (file)
	{
	  if(verbose)fvwm_msg(ERR,"ReadDecorFace",
			      "couldn't load pixmap %s", file);
	  free(file);
	}
	return False;
      }
      if (file)
      {
	free(file);
	file = NULL;
      }

      memset(&df->style, 0, sizeof(df->style));
      if (strncasecmp(style,"Tiled",5)==0)
	DFS_FACE_TYPE(df->style) = TiledPixmapButton;
      else
	DFS_FACE_TYPE(df->style) = PixmapButton;
    }
#ifdef MINI_ICONS
    else if (strncasecmp (style, "MiniIcon", 8) == 0)
    {
      memset(&df->style, 0, sizeof(df->style));
      DFS_FACE_TYPE(df->style) = MiniIconButton;
#if 0
/* Have to remove this again. This is all so badly written there is no chance
 * to prevent a coredump and a memory leak the same time without a rewrite of
 * large parts of the code. */
      if (df->u.p)
	DestroyPicture(dpy, df->u.p);
#endif
      df->u.p = NULL; /* pixmap read in when the window is created */
    }
#endif
    else
    {
      if(verbose)
	fvwm_msg(ERR,"ReadDecorFace", "unknown style %s: %s", style, action);
      return False;
    }
  }

  /* Process button flags ("--" signals start of flags,
     it is also checked for above) */
  s = GetNextToken(s, &file);
  if (file && (strcmp(file,"--")==0))
  {
    char *tok;
    s = GetNextToken(s, &tok);
    while (tok&&*tok)
    {
      int set = 1;

      if (*tok == '!')
      { /* flag negate */
	set = 0;
	++tok;
      }
      if (StrEquals(tok,"Clear"))
      {
	memset(&DFS_FLAGS(df->style), (set) ? 0 : 0xff,
	       sizeof(DFS_FLAGS(df->style)));
	/* ? what is set == 0 good for ? */
      }
      else if (StrEquals(tok,"Left"))
      {
	if (set)
	  DFS_H_JUSTIFICATION(df->style) = JUST_LEFT;
	else
	  DFS_H_JUSTIFICATION(df->style) = JUST_RIGHT;
      }
      else if (StrEquals(tok,"Right"))
      {
	if (set)
	  DFS_H_JUSTIFICATION(df->style) = JUST_RIGHT;
	else
	  DFS_H_JUSTIFICATION(df->style) = JUST_LEFT;
      }
      else if (StrEquals(tok,"Centered"))
      {
	DFS_H_JUSTIFICATION(df->style) = JUST_CENTER;
	DFS_V_JUSTIFICATION(df->style) = JUST_CENTER;
      }
      else if (StrEquals(tok,"Top"))
      {
	if (set)
	  DFS_V_JUSTIFICATION(df->style) = JUST_TOP;
	else
	  DFS_V_JUSTIFICATION(df->style) = JUST_BOTTOM;
      }
      else if (StrEquals(tok,"Bottom"))
      {
	if (set)
	  DFS_V_JUSTIFICATION(df->style) = JUST_BOTTOM;
	else
	  DFS_V_JUSTIFICATION(df->style) = JUST_TOP;
      }
      else if (StrEquals(tok,"Flat"))
      {
	if (set)
	  DFS_BUTTON_RELIEF(df->style) = DFS_BUTTON_IS_FLAT;
	else if (DFS_BUTTON_RELIEF(df->style) == DFS_BUTTON_IS_FLAT)
	  DFS_BUTTON_RELIEF(df->style) = DFS_BUTTON_IS_UP;
      }
      else if (StrEquals(tok,"Sunk"))
      {
	if (set)
	  DFS_BUTTON_RELIEF(df->style) = DFS_BUTTON_IS_SUNK;
	else if (DFS_BUTTON_RELIEF(df->style) == DFS_BUTTON_IS_SUNK)
	  DFS_BUTTON_RELIEF(df->style) = DFS_BUTTON_IS_UP;
      }
      else if (StrEquals(tok,"Raised"))
      {
	if (set)
	  DFS_BUTTON_RELIEF(df->style) = DFS_BUTTON_IS_UP;
	else
	  DFS_BUTTON_RELIEF(df->style) = DFS_BUTTON_IS_SUNK;
      }
      else if (StrEquals(tok,"UseTitleStyle"))
      {
	if (set)
	{
	  DFS_USE_TITLE_STYLE(df->style) = 1;
	  DFS_USE_BORDER_STYLE(df->style) = 0;
	}
	else
	  DFS_USE_TITLE_STYLE(df->style) = 0;
      }
      else if (StrEquals(tok,"HiddenHandles"))
      {
	DFS_HAS_HIDDEN_HANDLES(df->style) = !!set;
      }
      else if (StrEquals(tok,"NoInset"))
      {
	DFS_HAS_NO_INSET(df->style) = !!set;
      }
      else if (StrEquals(tok,"UseBorderStyle"))
      {
	if (set)
	{
	  DFS_USE_BORDER_STYLE(df->style) = 1;
	  DFS_USE_TITLE_STYLE(df->style) = 0;
	}
	else
	  DFS_USE_BORDER_STYLE(df->style) = 0;
      }
      else
	if(verbose)
	  fvwm_msg(ERR,"ReadDecorFace",
		   "unknown button face flag %s -- line: %s",
		   tok, action);
      if (set)
	free(tok);
      else
	free(tok - 1);
      s = GetNextToken(s, &tok);
    }
  }
  if (file)
    free(file);
  return True;
}


/*****************************************************************************
 *
 * Reads a title button description (veliaa@rpi.edu)
 *
 ****************************************************************************/
static char *ReadTitleButton(
    char *s, TitleButton *tb, Boolean append, int button)
{
  char *end = NULL, *spec;
  DecorFace tmpdf;
  enum ButtonState bs = MaxButtonState;
  int i = 0, all = 0, pstyle = 0;

  while(isspace((unsigned char)*s))++s;
  for (; i < MaxButtonState; ++i)
    if (strncasecmp(button_states[i],s,
		    strlen(button_states[i]))==0)
    {
      bs = i;
      break;
    }
  if (bs != MaxButtonState)
    s += strlen(button_states[bs]);
  else
    all = 1;
  while(isspace((unsigned char)*s))++s;
  if ('(' == *s)
  {
    int len;
    pstyle = 1;
    if (!(end = strchr(++s, ')')))
    {
      fvwm_msg(ERR,"ReadTitleButton",
	       "missing parenthesis: %s", s);
      return NULL;
    }
    while (isspace((unsigned char)*s)) ++s;
    len = end - s + 1;
    spec = safemalloc(len);
    strncpy(spec, s, len - 1);
    spec[len - 1] = 0;
  }
  else
    spec = s;

  while(isspace((unsigned char)*spec))
    ++spec;
  /* setup temporary in case button read fails */
  memset(&DFS_FLAGS(tmpdf.style), 0, sizeof(DFS_FLAGS(tmpdf.style)));
  DFS_FACE_TYPE(tmpdf.style) = SimpleButton;

#ifdef MULTISTYLE
  tmpdf.next = NULL;
#endif
#ifdef MINI_ICONS
  tmpdf.u.p = NULL;
#endif

  if (strncmp(spec, "--",2)==0)
  {
    /* only change flags */
    if (ReadDecorFace(spec, &TB_STATE(*tb)[(all) ? 0 : bs], button, True)
	&& all)
    {
      for (i = 0; i < MaxButtonState; ++i)
	ReadDecorFace(spec, &TB_STATE(*tb)[i],-1,False);
    }
  }
  else if (ReadDecorFace(spec, &tmpdf,button,True))
  {
    int b = all ? 0 : bs;
#ifdef MULTISTYLE
    if (append)
    {
      DecorFace *tail = &TB_STATE(*tb)[b];
      while (tail->next) tail = tail->next;
      tail->next = (DecorFace *)safemalloc(sizeof(DecorFace));
      *tail->next = tmpdf;
      if (all)
	for (i = 1; i < MaxButtonState; ++i)
	{
	  tail = &TB_STATE(*tb)[i];
	  while (tail->next) tail = tail->next;
	  tail->next = (DecorFace *)safemalloc(sizeof(DecorFace));
	  memset(&DFS_FLAGS(tail->next->style), 0,
		 sizeof(DFS_FLAGS(tail->next->style)));
	  DFS_FACE_TYPE(tail->next->style) = SimpleButton;
	  tail->next->next = NULL;
	  ReadDecorFace(spec, tail->next, button, False);
	}
    }
    else
    {
#endif
      FreeDecorFace(dpy, &TB_STATE(*tb)[b]);
      TB_STATE(*tb)[b] = tmpdf;
      if (all)
	for (i = 1; i < MaxButtonState; ++i)
	  ReadDecorFace(spec, &TB_STATE(*tb)[i],button,False);
#ifdef MULTISTYLE
    }
#endif

  }
  if (pstyle)
  {
    free(spec);
    ++end;
    while(isspace((unsigned char)*end))++end;
  }
  return end;
}


#ifdef USEDECOR
/*****************************************************************************
 *
 * Diverts a style definition to an FvwmDecor structure (veliaa@rpi.edu)
 *
 ****************************************************************************/
void AddToDecor(FvwmDecor *decor, char *s)
{
  if (!s)
    return;
  while (*s && isspace((unsigned char)*s))
    ++s;
  if (!*s)
    return;
  Scr.cur_decor = decor;
  ExecuteFunction(s, NULL, &Event, C_ROOT, -1, 0, NULL);
  Scr.cur_decor = NULL;
}

/*****************************************************************************
 *
 * Changes the window's FvwmDecor pointer (veliaa@rpi.edu)
 *
 ****************************************************************************/
void ChangeDecor(F_CMD_ARGS)
{
  char *item;
  int old_height;
  int extra_height;
  FvwmDecor *decor = &Scr.DefaultDecor;
  FvwmDecor *found = NULL;

  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
    return;
  item = PeekToken(action, &action);
  if (!action || !item)
    return;
  /* search for tag */
  for (; decor; decor = decor->next)
  {
    if (decor->tag)
    {
      if (StrEquals(item, decor->tag))
      {
	found = decor;
	break;
      }
    }
  }
  if (!found)
  {
    XBell(dpy, 0);
    return;
  }
  apply_decor_change(tmp_win);
  old_height = tmp_win->decor->title_height;
  tmp_win->decor = found;
  extra_height = (HAS_TITLE(tmp_win)) ?
    (old_height - tmp_win->decor->title_height) : 0;
  ForceSetupFrame(
    tmp_win, tmp_win->frame_g.x, tmp_win->frame_g.y, tmp_win->frame_g.width,
    tmp_win->frame_g.height - extra_height, True);
  DrawDecorations(tmp_win, DRAW_ALL, (Scr.Hilite == tmp_win), 2, None);
}

/*****************************************************************************
 *
 * Destroys an FvwmDecor (veliaa@rpi.edu)
 *
 ****************************************************************************/
void DestroyDecor(F_CMD_ARGS)
{
  char *item;
  FvwmDecor *decor = Scr.DefaultDecor.next;
  FvwmDecor *prev = &Scr.DefaultDecor, *found = NULL;

  action = GetNextToken(action, &item);
  if (!action || !item)
  {
    if (item)
      free(item);
    return;
  }

  /* search for tag */
  for (; decor; decor = decor->next)
  {
    if (decor->tag)
      if (StrEquals(item, decor->tag))
      {
	found = decor;
	break;
      }
    prev = decor;
  }
  free(item);

  if (found && (found != &Scr.DefaultDecor))
  {
    FvwmWindow *fw = Scr.FvwmRoot.next;
    while(fw)
    {
      if (fw->decor == found)
      {
	ExecuteFunction(
	  "ChangeDecor Default", fw, eventp, C_WINDOW, *Module, 0, NULL);
      }
      fw = fw->next;
    }
    prev->next = found->next;
    DestroyFvwmDecor(found);
    free(found);
  }
}

/***********************************************************************
 *
 *  InitFvwmDecor -- initializes an FvwmDecor structure to defaults
 *
 ************************************************************************/
void InitFvwmDecor(FvwmDecor *decor)
{
    int i;
    DecorFace tmpdf;

    decor->title_height = 0;

#ifdef USEDECOR
    decor->tag = NULL;
    decor->next = NULL;
#endif

    /* initialize title-bar button styles */
    memset(&tmpdf.style, 0, sizeof(tmpdf.style));
    DFS_FACE_TYPE(tmpdf.style) = SimpleButton;
#ifdef MULTISTYLE
    tmpdf.next = NULL;
#endif
    for (i = 0; i < NUMBER_OF_BUTTONS; ++i)
    {
      int j = 0;
      for (; j < MaxButtonState; ++j)
      {
	TB_STATE(decor->buttons[i])[j] = tmpdf;
      }
    }

    /* reset to default button set */
    ResetAllButtons(decor);

    /* initialize title-bar styles */
    memset(&TB_FLAGS(decor->titlebar), 0, sizeof(TB_FLAGS(decor->titlebar)));

    for (i = 0; i < MaxButtonState; ++i)
    {
      memset(&TB_STATE(decor->titlebar)[i].style, 0,
	     sizeof(TB_STATE(decor->titlebar)[i].style));
      DFS_FACE_TYPE(TB_STATE(decor->titlebar)[i].style) = SimpleButton;
#ifdef MULTISTYLE
      TB_STATE(decor->titlebar)[i].next = NULL;
#endif
    }

    /* initialize border texture styles */
    memset(&decor->BorderStyle.active.style, 0,
	   sizeof(decor->BorderStyle.active.style));
    DFS_FACE_TYPE(decor->BorderStyle.active.style) = SimpleButton;
    memset(&decor->BorderStyle.inactive.style, 0,
	   sizeof(decor->BorderStyle.inactive.style));
    DFS_FACE_TYPE(decor->BorderStyle.inactive.style) = SimpleButton;
#ifdef MULTISTYLE
    decor->BorderStyle.active.next = NULL;
    decor->BorderStyle.inactive.next = NULL;
#endif
}

/***********************************************************************
 *
 *  DestroyFvwmDecor -- frees all memory assocated with an FvwmDecor
 *	structure, but does not free the FvwmDecor itself
 *
 ************************************************************************/
void DestroyFvwmDecor(FvwmDecor *decor)
{
  int i;
  /* reset to default button set (frees allocated mem) */
  ResetAllButtons(decor);
  for (i = 0; i < 3; ++i)
  {
    int j = 0;
    for (; j < MaxButtonState; ++j)
      FreeDecorFace(dpy, &TB_STATE(decor->titlebar)[i]);
  }
  FreeDecorFace(dpy, &decor->BorderStyle.active);
  FreeDecorFace(dpy, &decor->BorderStyle.inactive);
#ifdef USEDECOR
  if (decor->tag)
  {
    free(decor->tag);
    decor->tag = NULL;
  }
#endif
}

/*****************************************************************************
 *
 * Initiates an AddToDecor (veliaa@rpi.edu)
 *
 ****************************************************************************/
void add_item_to_decor(F_CMD_ARGS)
{
    FvwmDecor *decor, *found = NULL;
    char *item = NULL, *s = action;

    s = GetNextToken(s, &item);

    if (!item)
      return;
    if (!s)
    {
      free(item);
      return;
    }
    /* search for tag */
    for (decor = &Scr.DefaultDecor; decor; decor = decor->next)
    {
      if (decor->tag)
      {
	if (StrEquals(item, decor->tag))
	{
	  found = decor;
	  break;
	}
      }
    }
    if (!found)
    { /* then make a new one */
      found = (FvwmDecor *)safemalloc(sizeof( FvwmDecor ));
      InitFvwmDecor(found);
      found->tag = item; /* tag it */
      /* add it to list */
      for (decor = &Scr.DefaultDecor; decor->next; decor = decor->next)
	;
      decor->next = found;
    }
    else
      free(item);

    if (found)
    {
	AddToDecor(found, s);

	/* Set + state to last decor */
	set_last_added_item(ADDED_DECOR, found);
    }
}
#endif /* USEDECOR */


/*****************************************************************************
 *
 * Updates window decoration styles (veliaa@rpi.edu)
 *
 ****************************************************************************/
void UpdateDecor(F_CMD_ARGS)
{
  FvwmWindow *fw = Scr.FvwmRoot.next;
#ifdef USEDECOR
  FvwmDecor *decor = &Scr.DefaultDecor, *found = NULL;
  FvwmWindow *hilight = Scr.Hilite;
  char *item = NULL;
  action = GetNextToken(action, &item);
  if (item)
  {
    /* search for tag */
    for (; decor; decor = decor->next)
      if (decor->tag)
	if (strcasecmp(item, decor->tag)==0)
	{
	  found = decor;
	  break;
	}
    free(item);
  }
#endif

  for (; fw; fw = fw->next)
  {
#ifdef USEDECOR
    /* update specific decor, or all */
    if (found)
    {
      if (fw->decor == found)
      {
	DrawDecorations(fw, DRAW_ALL, True, True, None);
	DrawDecorations(fw, DRAW_ALL, False, True, None);
      }
    }
    else
#endif
    {
      DrawDecorations(fw, DRAW_ALL, True, True, None);
      DrawDecorations(fw, DRAW_ALL, False, True, None);
    }
  }
  DrawDecorations(hilight, DRAW_ALL, True, True, None);
}

void reset_decor_changes(void)
{
#ifndef USEDECOR
  Scr.DefaultDecor.flags.has_changed = 0;
  Scr.DefaultDecor.flags.has_title_height_changed = 0;
#else
  FvwmDecor *decor;
  for (decor = &Scr.DefaultDecor; decor; decor = decor->next)
  {
    decor->flags.has_changed = 0;
    decor->flags.has_title_height_changed = 0;
  }
  /*!!! must reset individual change flags too */
#endif
}

/*****************************************************************************
 *
 * Changes a button decoration style (changes by veliaa@rpi.edu)
 *
 ****************************************************************************/
static void SetMWMButtonFlag(
  mwm_flags flag, int multi, int set, FvwmDecor *decor, TitleButton *tb)
{
  int i;
  int start = 0;
  int add = 2;

  if (multi)
  {
    if (multi == 2)
      start = 1;
    else if (multi == 3)
      add = 1;
    for (i = start; i < NUMBER_OF_BUTTONS; i += add)
    {
      if (set)
	TB_MWM_DECOR_FLAGS(decor->buttons[i]) |= flag;
      else
	TB_MWM_DECOR_FLAGS(decor->buttons[i]) &= ~flag;
    }
  }
  else
  {
    if (set)
      TB_MWM_DECOR_FLAGS(*tb) |= flag;
    else
      TB_MWM_DECOR_FLAGS(*tb) &= ~flag;
  }

  return;
}

static void do_button_style(F_CMD_ARGS, Bool do_add)
{
  int i;
  int multi = 0;
  int button = 0;
  char *text = NULL;
  char *prev = NULL;
  char *parm = NULL;
  TitleButton *tb = NULL;
#ifdef USEDECOR
  FvwmDecor *decor = Scr.cur_decor ? Scr.cur_decor : &Scr.DefaultDecor;
#else
  FvwmDecor *decor = &Scr.DefaultDecor;
#endif

  parm = PeekToken(action, &text);
  if (parm && isdigit(*parm))
  {
    button = atoi(parm);
    button = BUTTON_INDEX(button);
  }

  if (parm == NULL || button >= NUMBER_OF_BUTTONS || button < 0)
  {
    fvwm_msg(ERR,"ButtonStyle","Bad button style (1) in line %s",action);
    return;
  }

  Scr.flags.do_need_window_update = 1;

  if (!isdigit(*parm))
  {
    if (StrEquals(parm,"left"))
      multi = 1; /* affect all left buttons */
    else if (StrEquals(parm,"right"))
      multi = 2; /* affect all right buttons */
    else if (StrEquals(parm,"all"))
      multi = 3; /* affect all buttons */
    else
    {
      /* we're either resetting buttons or
	 an invalid button set was specified */
      if (StrEquals(parm,"reset"))
	ResetAllButtons(decor);
      else
	fvwm_msg(ERR,"ButtonStyle","Bad button style (2) in line %s", action);
      return;
    }
  }

  /* mark button style and decor as changed */
  decor->flags.has_changed = 1;

  if (multi == 0)
  {
    /* a single button was specified */
    tb = &decor->buttons[button];
    TB_FLAGS(*tb).has_changed = 1;
  }
  else
  {
    for (i = 0; i < NUMBER_OF_BUTTONS; ++i)
    {
      if (((multi & 1) && !(i & 1)) ||
	  ((multi & 2) && (i & 1)))
	TB_FLAGS(decor->buttons[i]).has_changed = 1;
    }
  }

  for (prev = text; (parm = PeekToken(text, &text)); prev = text)
  {
    if (!do_add && strcmp(parm,"-") == 0)
    {
      char *tok;
      text = GetNextToken(text, &tok);
      while (tok)
      {
	int set = 1;

	if (*tok == '!')
	{
	  /* flag negate */
	  set = 0;
	  ++tok;
	}
	if (StrEquals(tok,"Clear"))
	{
	  if (multi)
	  {
	    for (i = 0; i < NUMBER_OF_BUTTONS; ++i)
	    {
	      if (((multi & 1) && !(i & 1)) ||
		  ((multi & 2) && (i & 1)))
	      {
		TB_JUSTIFICATION(decor->buttons[i]) =
		  (set) ? JUST_CENTER : JUST_RIGHT;
		memset(&TB_FLAGS(decor->buttons[i]), (set) ? 0 : 0xff,
		       sizeof(TB_FLAGS(decor->buttons[i])));
		/* ? not very useful if set == 0 ? */
	      }
	    }
	  }
	  else
	  {
	    TB_JUSTIFICATION(*tb) = (set) ? JUST_CENTER : JUST_RIGHT;
	    memset(&TB_FLAGS(*tb), (set) ? 0 : 0xff, sizeof(TB_FLAGS(*tb)));
	    /* ? not very useful if set == 0 ? */
	  }
	}
	else if (StrEquals(tok, "MWMDecorMenu"))
	{
	  SetMWMButtonFlag(MWM_DECOR_MENU, multi, set, decor, tb);
	}
	else if (StrEquals(tok, "MWMDecorMin"))
	{
	  SetMWMButtonFlag(MWM_DECOR_MINIMIZE, multi, set, decor, tb);
	}
	else if (StrEquals(tok, "MWMDecorMax"))
	{
	  SetMWMButtonFlag(MWM_DECOR_MAXIMIZE, multi, set, decor, tb);
	}
	else if (StrEquals(tok, "MWMDecorShade"))
	{
	  SetMWMButtonFlag(MWM_DECOR_SHADE, multi, set, decor, tb);
	}
	else if (StrEquals(tok, "MWMDecorStick"))
	{
	  SetMWMButtonFlag(MWM_DECOR_STICK, multi, set, decor, tb);
	}
	else
	{
	  fvwm_msg(ERR, "ButtonStyle",
		   "unknown title button flag %s -- line: %s",
		   tok, text);
	}
	if (set)
	  free(tok);
	else
	  free(tok - 1);
	text = GetNextToken(text, &tok);
      }
      break;
    }
    else
    {
      if (multi)
      {
	for (i = 0; i < NUMBER_OF_BUTTONS; ++i)
	{
	  if (((multi & 1) && !(i & 1)) ||
	      ((multi & 2) && (i & 1)))
	  {
	    text = ReadTitleButton(prev, &decor->buttons[i], do_add, i);
	  }
	}
      }
      else if (!(text = ReadTitleButton(prev, tb, do_add, button)))
      {
	break;
      }
    }
  }
}

void ButtonStyle(F_CMD_ARGS)
{
  do_button_style(F_PASS_ARGS, False);
}

#ifdef MULTISTYLE
/*****************************************************************************
 *
 * Appends a button decoration style (veliaa@rpi.edu)
 *
 ****************************************************************************/
void AddButtonStyle(F_CMD_ARGS)
{
  do_button_style(F_PASS_ARGS, True);
}
#endif /* MULTISTYLE */


void SetEnv(F_CMD_ARGS)
{
  char *szVar = NULL;
  char *szValue = NULL;
  char *szPutenv = NULL;

  action = GetNextToken(action,&szVar);
  if (!szVar)
    return;
  action = GetNextToken(action,&szValue);
  if (!szValue)
  {
    free(szVar);
    return;
  }

  szPutenv = safemalloc(strlen(szVar)+strlen(szValue)+2);
  sprintf(szPutenv,"%s=%s",szVar,szValue);
  putenv(szPutenv);
  free(szVar);
  free(szValue);
}

static void do_recapture(F_CMD_ARGS, Bool fSingle)
{
  XEvent event;
  Bool need_ungrab = False;

  if (fSingle)
    if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
      return;
  /* FIXME: domivogt (1-Jun-2000): For some reason unknown to me the pointer
   * grab fails if an icon is focused.  So, to prevent fvwm from hanging, do not
   * use the BUSY_CURSOR in this case.  This is not a proper fix, but I am out
   * of ideas. */
  if ((!Scr.Focus || !IS_ICONIFIED(Scr.Focus)) &&
      (Scr.BusyCursor & BUSY_RECAPTURE))
  {
    if (GrabEm(CRS_WAIT, GRAB_BUSY))
      need_ungrab = True;
  }
  XSync(dpy,0);
  MyXGrabServer(dpy);
  XSync(dpy,0);
  if (fSingle)
    CaptureOneWindow(tmp_win, tmp_win->w);
  else
    CaptureAllWindows();
  /* Throw away queued up events. We don't want user input during a
   * recapture. The window the user clicks in might disapper at the very same
   * moment and the click goes through to the root window. Not good */
  while (XCheckMaskEvent(dpy, ButtonPressMask|ButtonReleaseMask|
			 ButtonMotionMask|PointerMotionMask|EnterWindowMask|
			 LeaveWindowMask|KeyPressMask|KeyReleaseMask,
			 &event) != False)
    ;
  MyXUngrabServer(dpy);
  if (need_ungrab)
    UngrabEm(GRAB_BUSY);
  XSync(dpy, 0);
}

void Recapture(F_CMD_ARGS)
{
  do_recapture(F_PASS_ARGS, False);
}

void RecaptureWindow(F_CMD_ARGS)
{
  do_recapture(F_PASS_ARGS, True);
}

void SetGlobalOptions(F_CMD_ARGS)
{
  char *opt;
  char *replace;
  Bool is_bugopt;

  fvwm_msg(ERR, "SetGlobalOptions", "The GlobalOpts command is obsolete.");

  /* fvwm_msg(DBG,"SetGlobalOptions","init action == '%s'\n",action); */
  for (action = GetNextSimpleOption(action, &opt); opt;
       action = GetNextSimpleOption(action, &opt))
  {
    /* fvwm_msg(DBG,"SetGlobalOptions"," opt == '%s'\n",opt); */
    /* fvwm_msg(DBG,"SetGlobalOptions"," remaining == '%s'\n",
       action?action:"(NULL)"); */
    replace = NULL;
    is_bugopt = False;
    if (StrEquals(opt,"WINDOWSHADESHRINKS"))
    {
      replace = "* WindowShadeShrinks";
    }
    else if (StrEquals(opt,"WINDOWSHADESCROLLS"))
    {
      replace = "* WindowShadeScrolls";
    }
    else if (StrEquals(opt,"SMARTPLACEMENTISREALLYSMART"))
    {
      replace = "* CleverPlacement";
    }
    else if (StrEquals(opt,"SMARTPLACEMENTISNORMAL"))
    {
      replace = "* SmartPlacement";
    }
    else if (StrEquals(opt,"CLICKTOFOCUSDOESNTPASSCLICK"))
    {
      replace = "* ClickToFocusPassesClickOff";
    }
    else if (StrEquals(opt,"CLICKTOFOCUSPASSESCLICK"))
    {
      replace = "* ClickToFocusPassesClick";
    }
    else if (StrEquals(opt,"clicktofocusdoesntraise"))
    {
      replace = "* ClickToFocusRaisesOff";
    }
    else if (StrEquals(opt,"CLICKTOFOCUSRAISES"))
    {
      replace = "* ClickToFocusRaises";
    }
    else if (StrEquals(opt,"MOUSEFOCUSCLICKDOESNTRAISE"))
    {
      replace = "* MouseFocusClickRaisesOff";
    }
    else if (StrEquals(opt,"MOUSEFOCUSCLICKRAISES"))
    {
      replace = "* MouseFocusClickRaises";
    }
    else if (StrEquals(opt,"NOSTIPLEDTITLES"))
    {
      replace = "* StippledTitleOff";
    }
    else if (StrEquals(opt,"STIPLEDTITLES"))
    {
      replace = "* StippledTitle";
    }
    else if (StrEquals(opt,"CAPTUREHONORSSTARTSONPAGE"))
    {
      replace = "* CaptureHonorsStartsOnPage";
    }
    else if (StrEquals(opt,"CAPTUREIGNORESSTARTSONPAGE"))
    {
      replace = "* CaptureIgnoresStartsOnPage";
    }
    else if (StrEquals(opt,"RECAPTUREHONORSSTARTSONPAGE"))
    {
      replace = "* RecaptureHonorsStartsOnPage";
    }
    else if (StrEquals(opt,"RECAPTUREIGNORESSTARTSONPAGE"))
    {
      replace = "* RecaptureIgnoresStartsOnPage";
    }
    else if (StrEquals(opt,"ACTIVEPLACEMENTHONORSSTARTSONPAGE"))
    {
      replace = "* ActivePlacementHonorsStartsOnPage";
    }
    else if (StrEquals(opt,"ACTIVEPLACEMENTIGNORESSTARTSONPAGE"))
    {
      replace = "* ActivePlacementIgnoresStartsOnPage";
    }
    else if (StrEquals(opt,"RAISEOVERNATIVEWINDOWS"))
    {
      is_bugopt = True;
      replace = "RaiseOverNativeWindows on";
    }
    else if (StrEquals(opt,"IGNORENATIVEWINDOWS"))
    {
      is_bugopt = True;
      replace = "RaiseOverNativeWindows off";
    }
    else
    {
      fvwm_msg(ERR,"SetGlobalOptions","Unknown Global Option '%s'",opt);
    }
    if (replace)
    {
      char *cmd;
      char *tmp;

      tmp = action;
      action = replace;
      if (!is_bugopt)
      {
        ProcessNewStyle(F_PASS_ARGS);
        cmd = "Style";
      }
      else
      {
        SetBugOptions(F_PASS_ARGS);
        cmd = "BugOpts";
      }
      action = tmp;
      fvwm_msg(
        ERR, "SetGlobalOptions",
        "Please replace 'GlobalOpts %s' with '%s %s'.", opt, cmd, replace);
    }
    /* should never be null, but checking anyways... */
    if (opt)
      free(opt);
  }
  if (opt)
    free(opt);
}

void SetBugOptions(F_CMD_ARGS)
{
  char *opt;
  char delim;
  int toggle;

  /* fvwm_msg(DBG,"SetGlobalOptions","init action == '%s'\n",action); */
  while (action)
  {
    action = DoGetNextToken(action, &opt, NULL, ",", &delim);
    if (!opt)
    {
      /* no more options */
      return;
    }
    if (delim == '\n' || delim == ',')
    {
      /* missing toggle argument */
      toggle = 2;
    }
    else
    {
      toggle = ParseToggleArgument(action, &action, 1, False);
    }

    if (StrEquals(opt, "FlickeringMoveWorkaround"))
    {
      switch (toggle)
      {
      case -1:
	Scr.bo.DisableConfigureNotify ^= 1;
	break;
      case 0:
      case 1:
	Scr.bo.DisableConfigureNotify = toggle;
	break;
      default:
#ifdef DISABLE_CONFIGURE_NOTIFY_DURING_MOVE
	Scr.bo.DisableConfigureNotify = 1;
#else
	Scr.bo.DisableConfigureNotify = 0;
#endif
	break;
      }
    }
    else if (StrEquals(opt, "MixedVisualWorkaround"))
    {
      switch (toggle)
      {
      case -1:
	Scr.bo.InstallRootCmap ^= 1;
	break;
      case 0:
      case 1:
	Scr.bo.InstallRootCmap = toggle;
	break;
      default:
	Scr.bo.InstallRootCmap = 0;
	break;
      }
    }
    else if (StrEquals(opt, "ModalityIsEvil"))
    {
      switch (toggle)
      {
      case -1:
	Scr.bo.ModalityIsEvil ^= 1;
	break;
      case 0:
      case 1:
	Scr.bo.ModalityIsEvil = toggle;
	break;
      default:
#ifdef MODALITY_IS_EVIL
	Scr.bo.ModalityIsEvil = 1;
#else
	Scr.bo.ModalityIsEvil = 0;
#endif
	break;
      }
    }
    else if (StrEquals(opt, "RaiseOverNativeWindows"))
    {
      switch (toggle)
      {
      case -1:
	Scr.bo.RaiseHackNeeded ^= 1;
	break;
      case 0:
      case 1:
	Scr.bo.RaiseHackNeeded = toggle;
	break;
      default:
	Scr.bo.RaiseHackNeeded = 0;
	break;
      }
    }
    else if (StrEquals(opt, "RaiseOverUnmanaged"))
    {
      switch (toggle)
      {
      case -1:
	Scr.bo.RaiseOverUnmanaged ^= 1;
	break;
      case 0:
      case 1:
	Scr.bo.RaiseOverUnmanaged = toggle;
	break;
      default:
	Scr.bo.RaiseOverUnmanaged = 0;
	break;
      }
    }
    else
    {
      fvwm_msg(ERR, "SetBugOptions", "Unknown Bug Option '%s'", opt);
    }
    free(opt);
  }
}

void Emulate(F_CMD_ARGS)
{
  char *style;

  GetNextToken(action, &style);
  if (!style || StrEquals(style, "fvwm"))
    {
      Scr.gs.EmulateMWM = False;
      Scr.gs.EmulateWIN = False;
    }
  else if (StrEquals(style, "mwm"))
    {
      Scr.gs.EmulateMWM = True;
      Scr.gs.EmulateWIN = False;
    }
  else if (StrEquals(style, "win"))
    {
      Scr.gs.EmulateMWM = False;
      Scr.gs.EmulateWIN = True;
    }
  else
    {
      fvwm_msg(ERR, "Emulate", "Unknown style '%s'", style);
    }
  free(style);
  Scr.flags.do_need_window_update = 1;
  Scr.flags.has_default_font_changed = 1;
  Scr.flags.has_default_color_changed = 1;
  return;
}
/*
 * The ColorLimit command is  ignored if the  user has no reason to limit
 * color.  This is so the   same configuration will work on  colorlimited
 * and   non-colorlimited  displays  without    resorting   to using    a
 * preprocessor.
 *
 * Lets assume  the display is  no more  than  2000x1000 pixels. Ie.  the
 * display can display no more than 2,000,000 million  pixels at once.  A
 * display depth of 21 will display 2  million colors at once.  Hence the
 * logic below.
 *
 * dje 03/22/99
 */
 /* It is also ignored if the colormap is static i.e you can't run out */
void SetColorLimit(F_CMD_ARGS)
{
  int val;

  /* from X.h:
   * Note that the statically allocated ones are even numbered and the
   * dynamically changeable ones are odd numbered */
  if (!(Pvisual->class & 1))
    return;
  if (Pdepth > 20) {               /* if more than 20 bit color */
    return;                             /* ignore the limit */
  }
  if (GetIntegerArguments(action, NULL, &val, 1) != 1)
  {
    fvwm_msg(ERR,"SetColorLimit","ColorLimit requires one argument");
    return;
  }

  Scr.ColorLimit = (long)val;
}


/* set animation parameters */
void set_animation(F_CMD_ARGS)
{
  char *opt;
  int delay;
  float pct;
  int i = 0;

  action = GetNextToken(action, &opt);
  if (!opt || sscanf(opt,"%d",&delay) != 1) {
    fvwm_msg(ERR,"SetAnimation",
	     "Improper milli-second delay as first argument");
    if (opt)
      free(opt);
    return;
  }
  free(opt);
  if (delay > 500) {
    fvwm_msg(WARN,"SetAnimation",
	     "Using longer than .5 seconds as between frame animation delay");
  }
  cmsDelayDefault = delay;
  action = GetNextToken(action, &opt);
  while (opt) {
    if (sscanf(opt,"%f",&pct) != 1) {
      fvwm_msg(ERR,"SetAnimation",
	       "Use fractional values ending in 1.0 as args 2 and on");
      free(opt);
      return;
    }
    rgpctMovementDefault[i++] = pct;
    free(opt);
    action = GetNextToken(action, &opt);
  }
  /* No pct entries means don't change them at all */
  if (i>0 && rgpctMovementDefault[i-1] != 1.0) {
    rgpctMovementDefault[i++] = 1.0;
  }
}

/* set the number or size of shade animation steps, N => steps, Np => pixels */
void setShadeAnim(F_CMD_ARGS)
{
  char *buf;

  if (!action)
    action = "";
  fvwm_msg(
    ERR, "setShadeAnim", "The WindowshadeAnimate command is obsolete. "
    "Please use 'Style * WindowShadeSteps %s' instead.", action);
  buf = safemalloc(strlen(action) + 32);
  sprintf(buf, "* WindowShadeSteps %s", action);
  action = buf;
  ProcessNewStyle(F_PASS_ARGS);
  free(buf);
  return;
}


/* A function to handle stroke (olicha Nov 11, 1999) */
#ifdef HAVE_STROKE
void strokeFunc(F_CMD_ARGS)
{
  int finished = 0;
  int abort = 0;
  int modifiers = eventp->xbutton.state;
  int start_event_type = eventp->type;
  char sequence[MAX_SEQUENCE+1];
  char *stroke_action;
  char *opt = NULL;
  Bool finish_on_release = True;
  KeySym keysym;
  Bool restor_repeat = False;
  Bool echo_sequence = False;
  Bool draw_motion = False;
  int i = 0;

  int* x = (int*)0;
  int* y = (int*)0;
  const int STROKE_CHUNK_SIZE = 0xff;
  int coords_size = STROKE_CHUNK_SIZE;

  Window JunkRoot, JunkChild;
  int JunkX, JunkY;
  unsigned int JunkMask;
  Bool feed_back = False;
  int stroke_width = 1;

  x = (int*)safemalloc(coords_size * sizeof(int));
  y = (int*)safemalloc(coords_size * sizeof(int));

  if(!GrabEm(CRS_STROKE, GRAB_NORMAL))
  {
    XBell(dpy, 0);
    return;
  }

  /* set the default option */
  if (eventp->type == KeyPress || eventp->type == ButtonPress)
    finish_on_release = True;
  else
    finish_on_release = False;

  /* parse the option */
  for (action = GetNextSimpleOption(action, &opt); opt;
       action = GetNextSimpleOption(action, &opt))
  {
    if (StrEquals("NotStayPressed",opt))
      finish_on_release = False;
    else if (StrEquals("EchoSequence",opt))
      echo_sequence = True;
    else if (StrEquals("DrawMotion",opt))
      draw_motion = True;
    else if (StrEquals("FeedBack",opt))
      feed_back = True;
    else if (StrEquals("StrokeWidth",opt))
    {
      /* stroke width takes a positive integer argument */
      if (opt)
        free(opt);
      action = GetNextToken(action, &opt);
      if (!opt)
	fvwm_msg(WARN,"StrokeWidth","needs an integer argument");
      /* we allow stroke_width == 0 which means drawing a `fast' line
       * of width 1; the upper level of 100 is arbitrary */
      else if (!sscanf(opt, "%d", &stroke_width) || stroke_width < 0 || stroke_width > 100) {
	fvwm_msg(WARN,"StrokeWidth","Bad integer argument %d", stroke_width);
	stroke_width = 1;
      }
    }
    else
      fvwm_msg(WARN,"StrokeFunc","Unknown option %s", opt);
    if (opt)
      free(opt);
  }
  if (opt)
    free(opt);

  /* Force auto repeat off and grab the Keyboard to get proper
   * KeyRelease events if we need it.
   * Some computers do not support keyRelease events, can we
   * check this here ? No ? */
  if (start_event_type == KeyPress && finish_on_release)
  {
    XKeyboardState kstate;

    XGetKeyboardControl(dpy, &kstate);
    if (kstate.global_auto_repeat == AutoRepeatModeOn)
    {
      XAutoRepeatOff(dpy);
      restor_repeat = True;
    }
    XGrabKeyboard(dpy, Scr.Root, False, GrabModeAsync, GrabModeAsync,
		  CurrentTime);
  }

  /* be ready to get a stroke sequence */
  stroke_init();

  if (draw_motion)
  {
    MyXGrabServer(dpy);
    XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild, &x[0], &y[0],
		&JunkX, &JunkY, &JunkMask);
    XSetLineAttributes(dpy,Scr.XorGC,stroke_width,LineSolid,CapButt,JoinMiter);
  }

  while (!finished && !abort)
  {
    /* block until there is an event */
    XMaskEvent(dpy,  ButtonPressMask | ButtonReleaseMask | KeyPressMask |
	       KeyReleaseMask | ButtonMotionMask | PointerMotionMask, eventp);
    /* Records the time */
    StashEventTime(eventp);

    switch (eventp->type)
    {
    case MotionNotify:
      stroke_record(eventp->xmotion.x,eventp->xmotion.y);
      if (draw_motion)
      {
	if ((x[i] != eventp->xmotion.x || y[i] != eventp->xmotion.y))
	{
	  i++;
	  if (i >= coords_size) {
	    coords_size += STROKE_CHUNK_SIZE;
	    x = (int*)realloc(x, coords_size * sizeof(int));
	    y = (int*)realloc(y, coords_size * sizeof(int));
	    if (!x || !y)
	    {
	      /* unlikely */
	      fvwm_msg(WARN,
		       "strokeFunc","unable to allocate %d bytes ... aborted.",
		       coords_size * sizeof(int));
	      abort = 1;
	      i = -1; /* no undrawing possible since either x or y == 0 */
	      break;
	    }
	  }
	  x[i] = eventp->xmotion.x;
	  y[i] = eventp->xmotion.y;
	  XDrawLine(dpy, Scr.Root, Scr.XorGC, x[i-1], y[i-1], x[i], y[i]);
	}
      }
      break;
    case ButtonRelease:
      if (finish_on_release && start_event_type == ButtonPress)
	finished = 1;
      break;
    case KeyRelease:
      if (finish_on_release &&  start_event_type == KeyPress)
	finished = 1;
      break;
    case KeyPress:
      keysym = XLookupKeysym(&eventp->xkey,0);
      /* abort if Escape or Delete is pressed (as in menus.c) */
      if (keysym == XK_Escape || keysym == XK_Delete ||
	  keysym == XK_KP_Separator)
	abort = 1;
      /* finish on enter or space (as in menus.c) */
       if (keysym == XK_Return || keysym == XK_KP_Enter ||
	   keysym ==  XK_space)
	 finished = 1;
       break;
    case ButtonPress:
     if (!finish_on_release)
       finished = 1;
     break;
    default:
      break;
    }
  }

  if (draw_motion)
  {
    while (i > 0)
    {
      XDrawLine(dpy, Scr.Root, Scr.XorGC, x[i-1], y[i-1], x[i], y[i]);
      i--;
    }
    XSetLineAttributes(dpy,Scr.XorGC,0,LineSolid,CapButt,JoinMiter);
    MyXUngrabServer(dpy);
    free(x);
    free(y);
  }
  if (start_event_type == KeyPress && finish_on_release)
    XUngrabKeyboard(dpy, CurrentTime);
  UngrabEm(GRAB_NORMAL);

  if (restor_repeat)
    XAutoRepeatOn(dpy);

  /* get the stroke sequence */
  stroke_trans(sequence);

  if (echo_sequence)
  {
    char num_seq[MAX_SEQUENCE+1];

    for(i=0;sequence[i] != '\0';i++)
    {
      /* Telephone to numeric pad */
      if ('7' <= sequence[i] && sequence[i] <= '9')
	num_seq[i] = sequence[i]-6;
      else if ('1' <= sequence[i] && sequence[i] <= '3')
	num_seq[i] = sequence[i]+6;
      else
	num_seq[i] = sequence[i];
    }
    num_seq[i++] = '\0';
    fvwm_msg(INFO, "StrokeFunc", "stroke sequence: %s (N%s)",
	     sequence, num_seq);
  }

  if (abort) return;

  /* check for a binding */
  stroke_action = CheckBinding(Scr.AllBindings, sequence, 0, modifiers,
			       GetUnusedModifiers(), context, STROKE_BINDING);

  /* execute the action */
  if (stroke_action != NULL)
  {
    if (feed_back && atoi(sequence) != 0)
    {
      GrabEm(CRS_WAIT, GRAB_BUSY);
      usleep(200000);
      UngrabEm(GRAB_BUSY);
    }
    ExecuteFunction(stroke_action, tmp_win, eventp, context, -1, 0, NULL);
  }

}
#endif /* HAVE_STROKE */
