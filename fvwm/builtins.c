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
#include <ctype.h>
#include <errno.h>
#include <X11/keysym.h>

#include "fvwm.h"
#include "events.h"
#include "style.h"
#include "icons.h"
#include "functions.h"
#include "menus.h"
#include "misc.h"
#include "screen.h"
#include "module_interface.h"
#include "stack.h"
#include "move_resize.h"
#include "virtual.h"
#include "defaults.h"
#include "gnome.h"
#include "borders.h"

static void ApplyIconFont(void);
static void ApplyWindowFont(FvwmDecor *fl);

static char *exec_shell_name="/bin/sh";
/* button state strings must match the enumerated states */
static char  *button_states[MaxButtonState]={
    "ActiveUp",
#ifdef ACTIVEDOWN_BTNS
    "ActiveDown",
#endif
#ifdef INACTIVE_BTNS
    "Inactive",
#endif
    "ToggledActiveUp",
#ifdef ACTIVEDOWN_BTNS
    "ToggledActiveDown",
#endif
#ifdef INACTIVE_BTNS
    "ToggledInactive",
#endif
};

static void move_sticky_window_to_same_page(
  int *x11, int *x12, int *y11, int *y12, int x21, int x22, int y21, int y22)
{
  /* make sure the x coordinate is on the same page as the reference window */
  if (*x11 >= x22)
  {
    while (*x11 >= x22)
    {
      *x11 -= Scr.MyDisplayWidth;
      *x12 -= Scr.MyDisplayWidth;
    }
  }
  else if (*x12 <= x21)
  {
    while (*x12 <= x21)
    {
      *x11 += Scr.MyDisplayWidth;
      *x12 += Scr.MyDisplayWidth;
    }
  }
  /* make sure the y coordinate is on the same page as the reference window */
  if (*y11 >= y22)
  {
    while (*y11 >= y22)
    {
      *y11 -= Scr.MyDisplayHeight;
      *y12 -= Scr.MyDisplayHeight;
    }
  }
  else if (*y12 <= y21)
  {
    while (*y12 <= y21)
    {
      *y11 += Scr.MyDisplayHeight;
      *y12 += Scr.MyDisplayHeight;
    }
  }

}

static void MaximizeHeight(FvwmWindow *win, unsigned int win_width, int win_x,
			   unsigned int *win_height, int *win_y)
{
  FvwmWindow *cwin;
  int x11, x12, x21, x22;
  int y11, y12, y21, y22;
  int new_y1, new_y2;
  Bool is_sticky;

  x11 = win_x;             /* Start x */
  y11 = *win_y;            /* Start y */
  x12 = x11 + win_width;   /* End x   */
  y12 = y11 + *win_height; /* End y   */
  new_y1 = truncate_to_multiple(y11, Scr.MyDisplayHeight);
  new_y2 = new_y1 + Scr.MyDisplayHeight;

  for (cwin = Scr.FvwmRoot.next; cwin; cwin = cwin->next) {
    if (IS_STICKY(cwin) || (IS_ICONIFIED(cwin) && IS_ICON_STICKY(cwin)))
      is_sticky = True;
    else
      is_sticky = False;
    if (cwin == win || (cwin->Desk != win->Desk && !is_sticky))
      continue;
    if (IS_ICONIFIED(cwin)) {
      if(cwin->icon_w == None || IS_ICON_UNMAPPED(cwin))
	continue;
      x21 = cwin->icon_x_loc;
      y21 = cwin->icon_y_loc;
      x22 = x21 + cwin->icon_p_width;
      y22 = y21 + cwin->icon_p_height + cwin->icon_w_height;
    } else {
      x21 = cwin->frame_g.x;
      y21 = cwin->frame_g.y;
      x22 = x21 + cwin->frame_g.width;
      y22 = y21 + cwin->frame_g.height;
    }
    if (is_sticky)
    {
      move_sticky_window_to_same_page(
	&x21, &x22, &new_y1, &new_y2, x11, x12, y11, y12);
    }

    /* Are they in the same X space? */
    if (!((x22 <= x11) || (x21 >= x12))) {
      if ((y22 <= y11) && (y22 >= new_y1)) {
	new_y1 = y22;
      } else if ((y12 <= y21) && (new_y2 >= y21)) {
	new_y2 = y21;
      }
    }
  }
  *win_height = new_y2 - new_y1;
  *win_y = new_y1;
}

static void MaximizeWidth(FvwmWindow *win, unsigned int *win_width, int *win_x,
			  unsigned int win_height, int win_y)
{
  FvwmWindow *cwin;
  int x11, x12, x21, x22;
  int y11, y12, y21, y22;
  int new_x1, new_x2;
  Bool is_sticky;

  x11 = *win_x;            /* Start x */
  y11 = win_y;             /* Start y */
  x12 = x11 + *win_width;  /* End x   */
  y12 = y11 + win_height;  /* End y   */
  new_x1 = truncate_to_multiple(x11, Scr.MyDisplayWidth);
  new_x2 = new_x1 + Scr.MyDisplayWidth;

  for (cwin = Scr.FvwmRoot.next; cwin; cwin = cwin->next) {
    if (IS_STICKY(cwin) || (IS_ICONIFIED(cwin) && IS_ICON_STICKY(cwin)))
      is_sticky = True;
    else
      is_sticky = False;
    if (cwin == win || (cwin->Desk != win->Desk && !is_sticky))
      continue;
    if (IS_ICONIFIED(cwin)) {
      if(cwin->icon_w == None || IS_ICON_UNMAPPED(cwin))
	continue;
      x21 = cwin->icon_x_loc;
      y21 = cwin->icon_y_loc;
      x22 = x21 + cwin->icon_p_width;
      y22 = y21 + cwin->icon_p_height + cwin->icon_w_height;
    } else {
      x21 = cwin->frame_g.x;
      y21 = cwin->frame_g.y;
      x22 = x21 + cwin->frame_g.width;
      y22 = y21 + cwin->frame_g.height;
    }
    if (is_sticky)
    {
      move_sticky_window_to_same_page(
	&new_x1, &new_x2, &y21, &y21, x11, x12, y11, y12);
    }

    /* Are they in the same Y space? */
    if (!((y22 <= y11) || (y21 >= y12))) {
      if ((x22 <= x11) && (x22 >= new_x1)) {
	new_x1 = x22;
      } else if ((x12 <= x21) && (new_x2 >= x21)) {
	new_x2 = x21;
      }
    }
  }
  *win_width  = new_x2 - new_x1;
  *win_x = new_x1;
}

/***********************************************************************
 *
 *  Procedure:
 *	(Un)Maximize a window.
 *
 ***********************************************************************/
void Maximize(F_CMD_ARGS)
{
  unsigned int new_width, new_height;
  int new_x,new_y;
  int page_x, page_y;
  int val1, val2, val1_unit, val2_unit;
  int toggle;
  char *token;
  char *taction;
  Bool grow_x = False;
  Bool grow_y = False;

  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
    return;

  if(tmp_win == NULL)
    return;

  if(check_if_function_allowed(F_MAXIMIZE,tmp_win,True,NULL) == 0)
  {
    XBell(dpy, 0);
    return;
  }
  toggle = ParseToggleArgument(action, &action, -1, 0);
  if (toggle == 0 && !IS_MAXIMIZED(tmp_win))
    return;

  if (toggle == 1 && IS_MAXIMIZED(tmp_win))
  {
    /* Fake that the window is not maximized. */
    SET_MAXIMIZED(tmp_win, 0);
  }

  /* parse first parameter */
  val1_unit = Scr.MyDisplayWidth;
  token = PeekToken(action, &taction);
  if (token && StrEquals(token, "grow"))
    {
      grow_x = True;
      val1 = 100;
      val1_unit = Scr.MyDisplayWidth;
    }
  else if (GetOnePercentArgument(action, &val1, &val1_unit) == 0)
    {
      val1 = 100;
      val1_unit = Scr.MyDisplayWidth;
    }

  /* parse second parameter */
  val2_unit = Scr.MyDisplayHeight;
  token = PeekToken(taction, NULL);
  if (token && StrEquals(token, "grow"))
    {
      grow_y = True;
      val2 = 100;
      val2_unit = Scr.MyDisplayHeight;
    }
  else if (GetOnePercentArgument(taction, &val2, &val2_unit) == 0)
    {
      val2 = 100;
      val2_unit = Scr.MyDisplayHeight;
    }

  if (IS_MAXIMIZED(tmp_win))
  {
    SET_MAXIMIZED(tmp_win, 0);
    /* Unmaximizing is slightly tricky since we want the window to
     * stay on the same page, even if we have moved to a different page
     * in the meantime. The orig values are absolute! */
    if (IsRectangleOnThisPage(&(tmp_win->frame_g), tmp_win->Desk))
      {
	/* Make sure we keep it on screen while unmaximizing. Since
	   orig_g is in absolute coords, we need to extract the
	   page-relative coords. This doesn't work well if the page
	   has been moved by a fractional part of the page size
	   between maximizing and unmaximizing. */
	new_x = tmp_win->orig_g.x -
	  truncate_to_multiple(tmp_win->orig_g.x,Scr.MyDisplayWidth);
	new_y = tmp_win->orig_g.y -
	  truncate_to_multiple(tmp_win->orig_g.y,Scr.MyDisplayHeight);
      }
    else
      {
	new_x = tmp_win->orig_g.x - Scr.Vx;
	new_y = tmp_win->orig_g.y - Scr.Vy;
      }
    new_width = tmp_win->orig_g.width;
    if (IS_SHADED(tmp_win))
      new_height = tmp_win->title_g.height + tmp_win->boundary_width;
    else
      new_height = tmp_win->orig_g.height;
    SetupFrame(tmp_win, new_x, new_y, new_width, new_height, TRUE, False);
    SetBorder(tmp_win, True, True, True, None);
  }
  else /* maximize */
  {
    if (IS_SHADED(tmp_win))
      new_height = tmp_win->orig_g.height;
    else
      new_height = tmp_win->frame_g.height;
    new_width = tmp_win->frame_g.width;

    if (IsRectangleOnThisPage(&(tmp_win->frame_g), tmp_win->Desk))
    {
      /* maximize on visible page */
      page_x = 0;
      page_y = 0;
    }
    else
    {
      /* maximize on the page where the center of the window is */
      page_x = truncate_to_multiple(
	(tmp_win->frame_g.x + tmp_win->frame_g.width-1) / 2,
	Scr.MyDisplayWidth);
      page_y = truncate_to_multiple(
	(tmp_win->frame_g.y + tmp_win->frame_g.height-1) / 2,
	Scr.MyDisplayHeight);
    }

    new_x = tmp_win->frame_g.x;
    new_y = tmp_win->frame_g.y;

    if (grow_y)
    {
      MaximizeHeight(tmp_win, new_width, new_x, &new_height, &new_y);
    }
    else if(val2 >0)
    {
      new_height = val2*val2_unit/100-2;
      new_y = page_y;
    }
    if (grow_x)
    {
      MaximizeWidth(tmp_win, &new_width, &new_x, new_height, new_y);
    }
    else if(val1 >0)
    {
      new_width = val1*val1_unit/100-2;
      new_x = page_x;
    }
    if((val1==0)&&(val2==0))
    {
      new_x = page_x;
      new_y = page_y;
      new_height = Scr.MyDisplayHeight-2;
      new_width = Scr.MyDisplayWidth-2;
    }
    SET_MAXIMIZED(tmp_win, 1);
    ConstrainSize (tmp_win, &new_width, &new_height, False, 0, 0);
    tmp_win->maximized_ht = new_height;
    if (IS_SHADED(tmp_win))
      new_height = tmp_win->frame_g.height;
    SetupFrame(tmp_win,new_x,new_y,new_width,new_height,TRUE,False);
    SetBorder(tmp_win,Scr.Hilite == tmp_win,True,True,None);
  }
}

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
 *  Builtin function: WindowShadeAnimate <steps>
 *      <steps> is number of steps in animation. If 0, the
 *              windowshade animation goes off. Default = 0.
 *
 ***********************************************************************/
void WindowShade(F_CMD_ARGS)
{
  int h, y, step=1, old_h;
  int new_x, new_y, new_height;
  int toggle;

  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
    return;
  if (tmp_win == NULL)
    return;

  if (!HAS_TITLE(tmp_win)) {
    XBell(dpy, 0);
    return;
  }

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

  /* calcuate the step size */
  if (Scr.shade_anim_steps > 0)
    step = tmp_win->orig_g.height/Scr.shade_anim_steps;
  else if (Scr.shade_anim_steps < 0) /* if it's -ve it means pixels */
    step = -Scr.shade_anim_steps;
  if (step <= 0)  /* We don't want an endless loop, do we? */
    step = 1;

  if ((IS_SHADED(tmp_win)) && toggle == 0)
  {
    /* unshade window */
    SET_SHADED(tmp_win, 0);
    new_x = tmp_win->frame_g.x;
    new_y = tmp_win->frame_g.y;
    if (IS_MAXIMIZED(tmp_win))
      new_height = tmp_win->maximized_ht;
    else
      new_height = tmp_win->orig_g.height;

    /* this is necessary if the maximized state has changed while shaded */
    SetupFrame(tmp_win, new_x, new_y, tmp_win->frame_g.width, new_height,
               True, True);

    if (Scr.shade_anim_steps != 0) {
      h = tmp_win->title_g.height+tmp_win->boundary_width;
      if (Scr.go.WindowShadeScrolls)
        XMoveWindow(dpy, tmp_win->w, 0, - (new_height-h));
      y = h - new_height;
      old_h = tmp_win->frame_g.height;
      while (h < new_height) {
        XResizeWindow(dpy, tmp_win->frame, tmp_win->frame_g.width, h);
        XResizeWindow(dpy, tmp_win->Parent,
                      tmp_win->frame_g.width - 2 * tmp_win->boundary_width,
                      max(h - 2 * tmp_win->boundary_width
                          - tmp_win->title_g.height, 1));
        if (Scr.go.WindowShadeScrolls)
          XMoveWindow(dpy, tmp_win->w, 0, y);
        tmp_win->frame_g.height = h;
        /* way too flickery
        SetBorder(tmp_win, tmp_win == Scr.Hilite, True, True, tmp_win->frame);
        */
        BroadcastConfig(M_CONFIGURE_WINDOW, tmp_win);
        FlushOutputQueues();
        XSync(dpy, 0);
        h+=step;
        y+=step;
      }
      tmp_win->frame_g.height = old_h;
      XMoveWindow(dpy, tmp_win->w, 0, 0);
    }
    SetupFrame(tmp_win, new_x, new_y, tmp_win->frame_g.width, new_height,
               True, False);
    BroadcastPacket(M_DEWINDOWSHADE, 3, tmp_win->w, tmp_win->frame,
                    (unsigned long)tmp_win);
  }
  else if (!IS_SHADED(tmp_win) && toggle == 1)
  {
    /* shade window */
    SET_SHADED(tmp_win, 1);

    if (Scr.shade_anim_steps != 0) {
      XLowerWindow(dpy, tmp_win->w);
      h = tmp_win->frame_g.height;
      y = 0;
      old_h = tmp_win->frame_g.height;
      while (h > tmp_win->title_g.height+tmp_win->boundary_width) {
	if (Scr.go.WindowShadeScrolls)
	  XMoveWindow(dpy, tmp_win->w, 0, y);
        XResizeWindow(dpy, tmp_win->frame, tmp_win->frame_g.width, h);
        XResizeWindow(dpy, tmp_win->Parent,
                      tmp_win->frame_g.width - 2 * tmp_win->boundary_width,
                      max(h - 2 * tmp_win->boundary_width
                          - tmp_win->title_g.height, 1));
        tmp_win->frame_g.height = h;
        /* way too flickery
	SetBorder(tmp_win, tmp_win == Scr.Hilite, True, True, tmp_win->frame);
        */
        BroadcastConfig(M_CONFIGURE_WINDOW, tmp_win);
        FlushOutputQueues();
        XSync(dpy, 0);
        h-=step;
        y-=step;
      }
      tmp_win->frame_g.height = old_h;
      if (Scr.go.WindowShadeScrolls)
        XMoveWindow(dpy, tmp_win->w, 0, 0);
    }
    SetupFrame(tmp_win, tmp_win->frame_g.x, tmp_win->frame_g.y,
               tmp_win->frame_g.width, tmp_win->title_g.height
               + tmp_win->boundary_width, False, False);
    BroadcastPacket(M_WINDOWSHADE, 3, tmp_win->w, tmp_win->frame,
                    (unsigned long)tmp_win);
  }
  SetBorder(tmp_win,Scr.Hilite == tmp_win,True,True,None);
  FlushOutputQueues();
  XSync(dpy, 0);

#ifdef GNOME
  GNOME_SetHints (tmp_win);
#endif
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

  XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
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
    Destroy(tmp_win);
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
    Destroy(tmp_win);
  else
    XKillClient(dpy, tmp_win->w);
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

void refresh_function(F_CMD_ARGS)
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;

#if 0
  valuemask = (CWBackPixel);
  attributes.background_pixel = 0;
#else /* CKH - i'd like to try this a little differently (clear window)*/
  valuemask = CWOverrideRedirect | CWBackingStore | CWSaveUnder | CWBackPixmap;
  attributes.override_redirect = True;
  attributes.save_under = False;
  attributes.background_pixmap = None;
#endif
  attributes.backing_store = NotUseful;
  w = XCreateWindow (dpy, Scr.Root, 0, 0,
		     (unsigned int) Scr.MyDisplayWidth,
		     (unsigned int) Scr.MyDisplayHeight,
		     (unsigned int) 0,
		     CopyFromParent, (unsigned int) CopyFromParent,
		     CopyFromParent, valuemask,
		     &attributes);
  XMapWindow (dpy, w);
  XDestroyWindow (dpy, w);
  XFlush (dpy);
}


void refresh_win_function(F_CMD_ARGS)
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;

  if (DeferExecution(eventp,&w,&tmp_win,&context,CRS_SELECT,ButtonRelease))
    return;

  valuemask = CWOverrideRedirect | CWBackingStore | CWSaveUnder | CWBackPixmap;
  attributes.override_redirect = True;
  attributes.save_under = False;
  attributes.background_pixmap = None;
  attributes.backing_store = NotUseful;
  w = XCreateWindow (dpy,
                     (context == C_ICON)?(tmp_win->icon_w):(tmp_win->frame),
                     0, 0,
		     (unsigned int) Scr.MyDisplayWidth,
		     (unsigned int) Scr.MyDisplayHeight,
		     (unsigned int) 0,
		     CopyFromParent, (unsigned int) CopyFromParent,
		     CopyFromParent, valuemask,
		     &attributes);
  XMapWindow (dpy, w);
  XDestroyWindow (dpy, w);
  XFlush (dpy);
}


void stick_function(F_CMD_ARGS)
{
  int toggle;

  if (DeferExecution(eventp,&w,&tmp_win,&context,CRS_SELECT,ButtonRelease))
    return;

  toggle = ParseToggleArgument(action, &action, -1, 0);
  if ((toggle == 1 && IS_STICKY(tmp_win)) ||
      (toggle == 0 && !IS_STICKY(tmp_win)))
    return;

  if(IS_STICKY(tmp_win))
  {
    SET_STICKY(tmp_win, 0);
  }
  else
  {
    if (tmp_win->Desk != Scr.CurrentDesk)
      do_move_window_to_desk(tmp_win, Scr.CurrentDesk);
    SET_STICKY(tmp_win, 1);
    move_window_doit(eventp, w, tmp_win, context, "", Module, FALSE, TRUE);
    /* move_window_doit resets the STICKY flag, so we must set it after the
     * call! */
    SET_STICKY(tmp_win, 1);
  }
  BroadcastConfig(M_CONFIGURE_WINDOW,tmp_win);
  SetTitleBar(tmp_win,(Scr.Hilite==tmp_win),True);

#ifdef GNOME
  GNOME_SetHints (tmp_win);
#endif
}

void wait_func(F_CMD_ARGS)
{
  Bool done = False;
  extern FvwmWindow *Tmp_win;

  while(!done)
  {
#if 0
    GrabEm(WAIT);
#endif
    if(My_XNextEvent(dpy, &Event))
    {
      DispatchEvent(False);
      if(Event.type == MapNotify)
      {
        if((Tmp_win)&&(matchWildcards(action,Tmp_win->name)==True))
          done = True;
        if((Tmp_win)&&(Tmp_win->class.res_class)&&
           (matchWildcards(action,Tmp_win->class.res_class)==True))
          done = True;
        if((Tmp_win)&&(Tmp_win->class.res_name)&&
           (matchWildcards(action,Tmp_win->class.res_name)==True))
          done = True;
      }
      else if (Event.type == KeyPress &&
	       XLookupKeysym(&(Event.xkey),0) == XK_Escape &&
	       Event.xbutton.state & ControlMask)
      {
	done = 1;
      }
    }
  }
#if 0
  UngrabEm();
#endif
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

void SetEdgeScroll(F_CMD_ARGS)
{
  int val1, val2, val1_unit,val2_unit,n;

  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
  if(n != 2)
  {
    fvwm_msg(ERR,"SetEdgeScroll","EdgeScroll requires two arguments");
    return;
  }

  /*
   * if edgescroll >1000 and < 100000m
   * wrap at edges of desktop (a "spherical" desktop)
   */
  if (val1 >= 1000)
  {
    val1 /= 1000;
    Scr.flags.edge_wrap_x = 1;
  }
  else
  {
    Scr.flags.edge_wrap_x = 0;
  }
  if (val2 >= 1000)
  {
    val2 /= 1000;
    Scr.flags.edge_wrap_y = 1;
  }
  else
  {
    Scr.flags.edge_wrap_y = 0;
  }

  Scr.EdgeScrollX = val1*val1_unit/100;
  Scr.EdgeScrollY = val2*val2_unit/100;

  checkPanFrames();
}

void SetEdgeResistance(F_CMD_ARGS)
{
  int val[2];

  if (GetIntegerArguments(action, NULL, val, 2) != 2)
  {
    fvwm_msg(ERR,"SetEdgeResistance","EdgeResistance requires two arguments");
    return;
  }

  Scr.ScrollResistance = val[0];
  Scr.MoveResistance = val[1];
}

void SetMoveThreshold(F_CMD_ARGS)
{
  int val = 0;

  if (GetIntegerArguments(action, NULL, &val, 1) < 1 || val < 0)
    Scr.MoveThreshold = DEFAULT_MOVE_THRESHOLD;
  Scr.MoveThreshold = val;
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


void SetSnapAttraction(F_CMD_ARGS)
{
  int val;
  char *token;

  if(GetIntegerArguments(action, &action, &val, 1) != 1)
  {
    Scr.SnapAttraction = DEFAULT_SNAP_ATTRACTION;
    Scr.SnapMode = DEFAULT_SNAP_ATTRACTION_MODE;
    return;
  }
  Scr.SnapAttraction = val;
  if (val < 0)
  {
    Scr.SnapAttraction = DEFAULT_SNAP_ATTRACTION;
  }

  action = GetNextToken(action, &token);
  if(token == NULL)
  {
    return;
  }

  Scr.SnapMode = -1;
  if(StrEquals(token,"All"))
  {
    Scr.SnapMode = 0;
  }
  else if(StrEquals(token,"SameType"))
  {
    Scr.SnapMode = 1;
  }
  else if(StrEquals(token,"Icons"))
  {
    Scr.SnapMode = 2;
  }
  else if(StrEquals(token,"Windows"))
  {
    Scr.SnapMode = 3;
  }

  if (Scr.SnapMode != -1)
  {
    free(token);
    action = GetNextToken(action, &token);
    if(token == NULL)
    {
      return;
    }
  }
  else
  {
    Scr.SnapMode = DEFAULT_SNAP_ATTRACTION_MODE;
  }

  if(StrEquals(token,"Screen"))
  {
    Scr.SnapMode += 8;
  }
  else
  {
    fvwm_msg(ERR,"SetSnapAttraction", "Invalid argument: %s", token);
  }

  free(token);
}

void SetSnapGrid(F_CMD_ARGS)
{
  int val[2];

  if(GetIntegerArguments(action, NULL, &val[0], 2) != 2)
  {
    Scr.SnapGridX = DEFAULT_SNAP_GRID_X;
    Scr.SnapGridY = DEFAULT_SNAP_GRID_Y;
    return;
  }

  Scr.SnapGridX = val[0];
  if(Scr.SnapGridX < 1)
  {
    Scr.SnapGridX = DEFAULT_SNAP_GRID_X;
  }
  Scr.SnapGridY = val[1];
  if(Scr.SnapGridY < 1)
  {
    Scr.SnapGridY = DEFAULT_SNAP_GRID_Y;
  }
}


void SetXOR(F_CMD_ARGS)
{
  int val;
  XGCValues gcv;
  unsigned long gcm;

  if(GetIntegerArguments(action, NULL, &val, 1) != 1)
  {
    val = 0;
  }

  gcm = GCFunction|GCLineWidth|GCForeground|GCFillStyle|GCSubwindowMode;
  gcv.subwindow_mode = IncludeInferiors;
  gcv.function = GXxor;
  gcv.line_width = 0;
  /* use passed in value, or try to calculate appropriate value if 0 */
  /* ctwm method: */
  /*
    gcv.foreground = (val1)?(val1):((((unsigned long) 1) << Scr.d_depth) - 1);
  */
  /* Xlib programming manual suggestion: */
  gcv.foreground = (val)?
    (val):(BlackPixel(dpy,Scr.screen) ^ WhitePixel(dpy,Scr.screen));
  gcv.fill_style = FillSolid;
  gcv.subwindow_mode = IncludeInferiors;

  /* modify DrawGC, only create once */
  if (Scr.DrawGC)
    XChangeGC(dpy, Scr.DrawGC, gcm, &gcv);
  else
    Scr.DrawGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  /* free up XORPixmap if possible */
  if (Scr.DrawPicture) {
    DestroyPicture(dpy, Scr.DrawPicture);
    Scr.DrawPicture = NULL;
  }
}


void SetXORPixmap(F_CMD_ARGS)
{
  char *PixmapName;
  Picture *GCPicture;
  XGCValues gcv;
  unsigned long gcm;

  action = GetNextToken(action, &PixmapName);
  if(PixmapName == NULL)
  {
    /* return to default value. */
    SetXOR(eventp, w, tmp_win, context, "0", Module);
    return;
  }

  /* search for pixmap */
  GCPicture = CachePicture(dpy, Scr.NoFocusWin, NULL, PixmapName,
			   Scr.ColorLimit);
  if (GCPicture == NULL) {
    fvwm_msg(ERR,"SetXORPixmap","Can't find pixmap %s", PixmapName);
    free(PixmapName);
    return;
  }
  free(PixmapName);

  /* free up old one */
  if (Scr.DrawPicture)
    DestroyPicture(dpy, Scr.DrawPicture);
  Scr.DrawPicture = GCPicture;

  /* create Graphics context */
  gcm = GCFunction|GCLineWidth|GCTile|GCFillStyle|GCSubwindowMode;
  gcv.subwindow_mode = IncludeInferiors;
  gcv.function = GXxor;
  gcv.line_width = 0;
  gcv.tile = GCPicture->picture;
  gcv.fill_style = FillTiled;
  gcv.subwindow_mode = IncludeInferiors;
  /* modify DrawGC, only create once */
  if (Scr.DrawGC)
    XChangeGC(dpy, Scr.DrawGC, gcm, &gcv);
  else
    Scr.DrawGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);
}

void SetOpaque(F_CMD_ARGS)
{
  int val;

  if(GetIntegerArguments(action, NULL, &val, 1) != 1)
  {
    Scr.OpaqueSize = DEFAULT_OPAQUE_MOVE_SIZE;
  }
  else
  {
    Scr.OpaqueSize = val;
  }
}


void SetDeskSize(F_CMD_ARGS)
{
  int val[2];

  if (GetIntegerArguments(action, NULL, val, 2) != 2 &&
      GetRectangleArguments(action, &val[0], &val[1]) != 2)
  {
    fvwm_msg(ERR,"SetDeskSize","DesktopSize requires two arguments");
    return;
  }

  Scr.VxMax = (val[0] <= 0)? 0: val[0]*Scr.MyDisplayWidth-Scr.MyDisplayWidth;
  Scr.VyMax = (val[1] <= 0)? 0: val[1]*Scr.MyDisplayHeight-Scr.MyDisplayHeight;
  BroadcastPacket(M_NEW_PAGE, 5,
                  Scr.Vx, Scr.Vy, Scr.CurrentDesk, Scr.VxMax, Scr.VyMax);

  checkPanFrames();

#ifdef GNOME
  /* update GNOME pager */
  GNOME_SetAreaCount();
#endif
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


char *ModulePath = FVWM_MODULEDIR;

void setModulePath(F_CMD_ARGS)
{
    static int need_to_free = 0;
    setPath( &ModulePath, action, need_to_free );
    need_to_free = 1;
}

FvwmDecor *cur_decor = NULL;
void SetHiColor(F_CMD_ARGS)
{
  XGCValues gcv;
  unsigned long gcm;
  char *hifore=NULL, *hiback=NULL;
  FvwmWindow *hilight;
#ifdef USEDECOR
  FvwmDecor *fl = cur_decor ? cur_decor : &Scr.DefaultDecor;
#else
  FvwmDecor *fl = &Scr.DefaultDecor;
#endif

  action = GetNextToken(action,&hifore);
  GetNextToken(action,&hiback);
  if(Pdepth > 2)
  {
    if(hifore)
    {
	fl->HiColors.fore = GetColor(hifore);
    }
    if(hiback)
    {
	fl->HiColors.back = GetColor(hiback);
    }
    fl->HiRelief.back  = GetShadow(fl->HiColors.back);
    fl->HiRelief.fore  = GetHilite(fl->HiColors.back);
  }
  else
  {
    fl->HiColors.back  = GetColor("white");
    fl->HiColors.fore  = GetColor("black");
    fl->HiRelief.back  = GetColor("black");
    fl->HiRelief.fore  = GetColor("white");
  }
  if (hifore) free(hifore);
  if (hiback) free(hiback);
  gcm = GCFunction|GCLineWidth|GCForeground|GCBackground;
  gcv.foreground = fl->HiRelief.fore;
  gcv.background = fl->HiRelief.back;
  gcv.function = GXcopy;
  gcv.line_width = 0;
  if(fl->HiReliefGC)
    XChangeGC(dpy, fl->HiReliefGC, gcm, &gcv);
  else
    fl->HiReliefGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  gcv.foreground = fl->HiRelief.back;
  gcv.background = fl->HiRelief.fore;
  if(fl->HiShadowGC)
    XChangeGC(dpy, fl->HiShadowGC, gcm, &gcv);
  else
    fl->HiShadowGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  if(Scr.flags.windows_captured && Scr.Hilite)
  {
    hilight = Scr.Hilite;
    SetBorder(Scr.Hilite,False,True,True,None);
    SetBorder(hilight,True,True,True,None);
  }
}


char *ReadTitleButton(char *s, TitleButton *tb, Boolean append, int button);

#if defined(MULTISTYLE) && defined(EXTENDED_TITLESTYLE)
/*****************************************************************************
 *
 * Appends a titlestyle (veliaa@rpi.edu)
 *
 ****************************************************************************/
void AddTitleStyle(F_CMD_ARGS)
{
#ifdef USEDECOR
    FvwmDecor *fl = cur_decor ? cur_decor : &Scr.DefaultDecor;
#else
    FvwmDecor *fl = &Scr.DefaultDecor;
#endif
    char *parm=NULL;

    /* See if there's a next token.  We actually don't care what it is, so
       GetNextToken allocating memory is overkill, but... */
    GetNextToken(action,&parm);
    while (parm)
    {
      free(parm);
      if ((action = ReadTitleButton(action, &fl->titlebar, True, -1)) == NULL)
        break;
      GetNextToken(action, &parm);
    }
}
#endif /* MULTISTYLE && EXTENDED_TITLESTYLE */

void SetTitleStyle(F_CMD_ARGS)
{
  char *parm=NULL, *prev = action;
#ifdef USEDECOR
  FvwmDecor *fl = cur_decor ? cur_decor : &Scr.DefaultDecor;
#else
  FvwmDecor *fl = &Scr.DefaultDecor;
#endif

  action = GetNextToken(action,&parm);
  while(parm)
  {
    if (StrEquals(parm,"centered"))
    {
      fl->titlebar.flags &= ~HOffCenter;
    }
    else if (StrEquals(parm,"leftjustified"))
    {
      fl->titlebar.flags |= HOffCenter;
      fl->titlebar.flags &= ~HRight;
    }
    else if (StrEquals(parm,"rightjustified"))
    {
      fl->titlebar.flags |= HOffCenter | HRight;
    }
#ifdef EXTENDED_TITLESTYLE
    else if (StrEquals(parm,"height"))
    {
	int height, next;
	if ( sscanf(action, "%d%n", &height, &next) > 0
	     && height > 4
	     && height <= 256)
	{
	    int x,y,w,h,extra_height;
	    FvwmWindow *tmp = Scr.FvwmRoot.next, *hi = Scr.Hilite;

	    extra_height = fl->TitleHeight;
	    fl->TitleHeight = height;
	    extra_height -= fl->TitleHeight;

#ifdef I18N_MB
	    fl->WindowFont.y = fl->WindowFont.font->ascent
		+ (height - (fl->WindowFont.height + 3)) / 2;
#else
	    fl->WindowFont.y = fl->WindowFont.font->ascent
		+ (height - (fl->WindowFont.font->ascent
			     + fl->WindowFont.font->descent + 3)) / 2;
#endif
	    if (fl->WindowFont.y < fl->WindowFont.font->ascent)
		fl->WindowFont.y = fl->WindowFont.font->ascent;

	    tmp = Scr.FvwmRoot.next;
	    hi = Scr.Hilite;
	    while(tmp)
	    {
		if (!HAS_TITLE(tmp)
#ifdef USEDECOR
		    || (tmp->fl != fl)
#endif
		    ) {
		    tmp = tmp->next;
		    continue;
		}
		x = tmp->frame_g.x;
		y = tmp->frame_g.y;
		w = tmp->frame_g.width;
		h = tmp->frame_g.height-extra_height;
		tmp->frame_g.x = 0;
		tmp->frame_g.y = 0;
		tmp->frame_g.height = 0;
		tmp->frame_g.width = 0;
		SetupFrame(tmp,x,y,w,h,True,False);
		SetTitleBar(tmp,True,True);
		SetTitleBar(tmp,False,True);
		tmp = tmp->next;
	    }
	    SetTitleBar(hi,True,True);
	}
	else
	    fvwm_msg(ERR,"SetTitleStyle",
		     "bad height argument (height must be from 5 to 256)");
	action += next;
    }
    else
    {
	if (!(action = ReadTitleButton(prev, &fl->titlebar, False, -1))) {
	    free(parm);
	    break;
	}
    }
#else /* ! EXTENDED_TITLESTYLE */
    else if (strcmp(parm,"--")==0) {
	if (!(action = ReadTitleButton(prev, &fl->titlebar, False, -1))) {
	    free(parm);
	    break;
	}
    }
#endif /* EXTENDED_TITLESTYLE */
    free(parm);
    prev = action;
    action = GetNextToken(action,&parm);
  }
} /* SetTitleStyle */

static void ApplyDefaultFontAndColors(void)
{
  XGCValues gcv;
  unsigned long gcm;
  int wid;
  int hei;

  Scr.StdFont.y = Scr.StdFont.font->ascent;
#ifndef I18N_MB
  Scr.StdFont.height = Scr.StdFont.font->ascent + Scr.StdFont.font->descent;
#endif

  /* make GC's */
  gcm = GCFunction|GCFont|GCLineWidth|GCForeground|GCBackground;
  gcv.function = GXcopy;
  gcv.font = Scr.StdFont.font->fid;
  gcv.line_width = 0;
  gcv.foreground = Scr.StdColors.fore;
  gcv.background = Scr.StdColors.back;
  if(Scr.StdGC)
    XChangeGC(dpy, Scr.StdGC, gcm, &gcv);
  else
    Scr.StdGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  gcm = GCFunction|GCLineWidth|GCForeground;
  gcv.foreground = Scr.StdRelief.fore;
  if(Scr.StdReliefGC)
    XChangeGC(dpy, Scr.StdReliefGC, gcm, &gcv);
  else
    Scr.StdReliefGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  gcv.foreground = Scr.StdRelief.back;
  if(Scr.StdShadowGC)
    XChangeGC(dpy, Scr.StdShadowGC, gcm, &gcv);
  else
    Scr.StdShadowGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  /* update the geometry window for move/resize */
  if(Scr.SizeWindow != None)
  {
    Scr.SizeStringWidth = XTextWidth(Scr.StdFont.font, " +8888 x +8888 ", 15);
    wid = Scr.SizeStringWidth + 2 * SIZE_HINDENT;
    hei = Scr.StdFont.height + 2 * SIZE_VINDENT;
    SetWindowBackground(dpy, Scr.SizeWindow, wid, hei,
			Scr.bg, Pdepth, Scr.StdGC);
    if(Scr.gs.EmulateMWM)
    {
      XMoveResizeWindow(dpy,Scr.SizeWindow,
			(Scr.MyDisplayWidth - wid)/2,
			(Scr.MyDisplayHeight - hei)/2, wid, hei);
    }
    else
    {
      XMoveResizeWindow(dpy,Scr.SizeWindow,0, 0, wid,hei);
    }
  }

  if (Scr.flags.has_icon_font == 0)
  {
#ifdef I18N_MB
    Scr.IconFont.fontset = Scr.StdFont.fontset;
    Scr.IconFont.height = Scr.StdFont.height;
#endif
    Scr.IconFont.font = Scr.StdFont.font;
    ApplyIconFont();
  }

  if (Scr.flags.has_window_font == 0)
  {
#ifdef I18N_MB
    Scr.DefaultDecor.WindowFont.fontset = Scr.StdFont.fontset;
    Scr.DefaultDecor.WindowFont.height = Scr.StdFont.height;
#endif
    Scr.DefaultDecor.WindowFont.font = Scr.StdFont.font;
    ApplyWindowFont(&Scr.DefaultDecor);
  }

  UpdateAllMenuStyles();

/* inform modules of colorset changes goes here
  BroadcastLook();
*/

}

void SetDefaultIcon(F_CMD_ARGS)
{
  if (Scr.DefaultIcon)
    free(Scr.DefaultIcon);
  GetNextToken(action, &Scr.DefaultIcon);
}


void SetDefaultBackground(F_CMD_ARGS)
{
  char *type = NULL;

  action = GetNextToken(action, &type);

  /* no args mean restore what was set with DefaultColors */
  if (!type) {
    if (Scr.bg->type.bits.is_pixmap)
      XFreePixmap(dpy, Scr.bg->pixmap);
    Scr.bg->pixmap = (Pixmap)Scr.StdColors.back;
    Scr.bg->type.word = 0;
  } else {

    if (StrEquals(type, "TiledPixmap") || StrEquals(type, "Pixmap")) {
      Picture *pic;
      char *name;

      action = GetNextToken(action, &name);
      if (!name) {
        fvwm_msg(ERR, "DefaultBackground", "%s: No pixmap specified", type);
        free(type);
        return;
      }

      pic = CachePicture(dpy, Scr.NoFocusWin, NULL, name, Scr.ColorLimit);

      if (pic && pic->depth == Pdepth) {
	Pixmap pixmap = XCreatePixmap(dpy, Scr.NoFocusWin, pic->width,
				      pic->height, Pdepth);

	XCopyArea(dpy, pic->picture, pixmap, Scr.StdGC, 0, 0, pic->width,
		  pic->height, 0, 0);
	if (Scr.bg->type.bits.is_pixmap)
	  XFreePixmap(dpy, Scr.bg->pixmap);
	Scr.bg->pixmap = pixmap;
	Scr.bg->type.bits.w = pic->width;
	Scr.bg->type.bits.h = pic->height;
	Scr.bg->type.bits.is_pixmap = True;
	if (StrEquals(type, "Pixmap"))
	  Scr.bg->type.bits.stretch_h = Scr.bg->type.bits.stretch_v = True;
	else
	  Scr.bg->type.bits.stretch_h = Scr.bg->type.bits.stretch_v = False;
      } else {
        fvwm_msg(ERR, "DefaultBackground", "Couldn't load pixmap %s\n", name);
      }
      if (pic) DestroyPicture(dpy, pic);

#ifdef GRADIENT_BUTTONS
    } else if (StrEquals(type + 1, "Gradient")) {
      unsigned int width, height;
      char vtype = type[0];
      Pixmap pixmap = CreateGradientPixmap(dpy, Scr.NoFocusWin, Scr.ScratchGC3,
					   vtype, action, Scr.bestTileWidth,
					   Scr.bestTileHeight, &width, &height);
      if (pixmap) {
	if (Scr.bg->type.bits.is_pixmap)
	  XFreePixmap(dpy, Scr.bg->pixmap);
	Scr.bg->pixmap = pixmap;
	Scr.bg->type.bits.w = width;
	Scr.bg->type.bits.h = height;
	Scr.bg->type.bits.is_pixmap = True;
	Scr.bg->type.bits.stretch_h = Scr.bg->type.bits.stretch_v = True;
	if ((vtype == 'H') || (vtype == 'h'))
	  Scr.bg->type.bits.stretch_v = False;
	else if ((vtype == 'V') || (vtype == 'v'))
	  Scr.bg->type.bits.stretch_h = False;
      }


#endif
    } else {
      fvwm_msg(ERR, "DefaultBackground", "unknown type %s %s", type,
	       action ? action : "");
    }

    free(type);
  }

  ApplyDefaultFontAndColors();
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
    back = strdup("black");

  if (!StrEquals(fore, "-"))
    {
      FreeColors(&Scr.StdColors.fore, 1);
      Scr.StdColors.fore = GetColor(fore);
    }
  if (!StrEquals(back, "-"))
    {
      FreeColors(&Scr.StdColors.back, 1);
      FreeColors(&Scr.StdRelief.back, 1);
      FreeColors(&Scr.StdRelief.fore, 1);
      Scr.StdColors.back = GetColor(back);
      Scr.StdRelief.fore = GetHilite(Scr.StdColors.back);
      Scr.StdRelief.back = GetShadow(Scr.StdColors.back);
      if (!Scr.bg->type.bits.is_pixmap)
        Scr.bg->pixmap = (Pixmap)Scr.StdColors.back;
    }
  free(fore);
  free(back);

  ApplyDefaultFontAndColors();
}

void LoadDefaultFont(F_CMD_ARGS)
{
  char *font;
  XFontStruct *xfs = NULL;
#ifdef I18N_MB
  XFontSet xfset;
  XFontSetExtents *fset_extents;
  XFontStruct **fs_list;
  char **ml;
#endif

  action = GetNextToken(action,&font);
  if (!font)
  {
    /* Try 'fixed' */
    font = strdup("");
  }

#ifdef I18N_MB
  if ((xfset = GetFontSetOrFixed(dpy, font)) == NULL)
#else
  if ((xfs = GetFontOrFixed(dpy, font)) == NULL)
#endif
  {
    fvwm_msg(ERR,"SetDefaultFont","Couldn't load font '%s' or 'fixed'\n",
	     font);
    free(font);
#ifdef I18N_MB
    if (Scr.StdFont.fontset == NULL)
#else
    if (Scr.StdFont.font == NULL)
#endif
      exit(1);
    else
      return;
  }
  free(font);
#ifdef I18N_MB
  if (Scr.StdFont.fontset != NULL)
    XFreeFontSet(dpy, Scr.StdFont.fontset);
  Scr.StdFont.fontset = xfset;

  /* backward compatiblity setup */
  XFontsOfFontSet(xfset, &fs_list, &ml);
  Scr.StdFont.font = fs_list[0];
  fset_extents = XExtentsOfFontSet(xfset);
  Scr.StdFont.height = fset_extents->max_logical_extent.height;
#else
  if (Scr.StdFont.font)
    XFreeFont(dpy, Scr.StdFont.font);
  Scr.StdFont.font = xfs;
#endif

  ApplyDefaultFontAndColors();
}

static void ApplyIconFont(void)
{
  FvwmWindow *tmp;

#ifndef I18N_MB
  Scr.IconFont.height = Scr.IconFont.font->ascent+Scr.IconFont.font->descent;
#endif
  Scr.IconFont.y = Scr.IconFont.font->ascent;

  tmp = Scr.FvwmRoot.next;
  while(tmp)
  {
    RedoIconName(tmp);

    if(IS_ICONIFIED(tmp))
    {
      DrawIconWindow(tmp);
    }
    tmp = tmp->next;
  }
}

void LoadIconFont(F_CMD_ARGS)
{
  char *font;
  XFontStruct *newfont;
#ifdef I18N_MB
  XFontSet newfontset;
#endif

  action = GetNextToken(action,&font);
  if (!font)
  {
    if (Scr.flags.has_icon_font == 1)
    {
      /* reset to default font */
      XFreeFont(dpy, Scr.IconFont.font);
      Scr.flags.has_icon_font = 0;
    }
#ifdef I18N_MB
    Scr.IconFont.fontset = Scr.StdFont.fontset;
    Scr.IconFont.height = Scr.StdFont.height;
#endif
    Scr.IconFont.font = Scr.StdFont.font;
    ApplyIconFont();
    return;
  }

#ifdef I18N_MB
  if ((newfontset = GetFontSetOrFixed(dpy, font))!=NULL)
  {
    XFontSetExtents *fset_extents;
    XFontStruct **fs_list;
    char **ml;

    if (Scr.IconFont.fontset != NULL && Scr.flags.has_icon_font == 1)
      XFreeFontSet(dpy, Scr.IconFont.fontset);
    Scr.flags.has_icon_font = 1;
    Scr.IconFont.fontset = newfontset;

    /* backward compatiblity setup */
    XFontsOfFontSet(newfontset, &fs_list, &ml);
    Scr.IconFont.font = fs_list[0];
    fset_extents = XExtentsOfFontSet(newfontset);
    Scr.IconFont.height = fset_extents->max_logical_extent.height;
#else
  if ((newfont = GetFontOrFixed(dpy, font)))
  {
    if (Scr.IconFont.font && Scr.flags.has_icon_font == 1)
      XFreeFont(dpy, Scr.IconFont.font);
    Scr.flags.has_icon_font = 1;
    Scr.IconFont.font = newfont;
#endif
    ApplyIconFont();
  }
  else
  {
    fvwm_msg(ERR,"LoadIconFont","Couldn't load font '%s' or 'fixed'\n", font);
  }
  free(font);
}

static void ApplyWindowFont(FvwmDecor *fl)
{
  FvwmWindow *tmp;
  FvwmWindow *hi;
  int x,y,w,h,extra_height;

#ifndef I18N_MB
  fl->WindowFont.height =
    fl->WindowFont.font->ascent+fl->WindowFont.font->descent;
#endif
  fl->WindowFont.y = fl->WindowFont.font->ascent;
  extra_height = fl->TitleHeight;
#ifdef I18N_MB
  fl->TitleHeight=fl->WindowFont.height+3;
#else
  fl->TitleHeight=fl->WindowFont.font->ascent+fl->WindowFont.font->descent+3;
#endif
  extra_height -= fl->TitleHeight;

  tmp = Scr.FvwmRoot.next;
  hi = Scr.Hilite;
  while(tmp)
  {
    if (!HAS_TITLE(tmp)
#ifdef USEDECOR
	|| (tmp->fl != fl)
#endif
      )
    {
      tmp = tmp->next;
      continue;
    }
    x = tmp->frame_g.x;
    y = tmp->frame_g.y;
    w = tmp->frame_g.width;
    h = tmp->frame_g.height-extra_height;
    tmp->frame_g.x = 0;
    tmp->frame_g.y = 0;
    tmp->frame_g.height = 0;
    tmp->frame_g.width = 0;
    SetupFrame(tmp,x,y,w,h,True,False);
    SetTitleBar(tmp,True,True);
    SetTitleBar(tmp,False,True);
    tmp = tmp->next;
  }
  SetTitleBar(hi,True,True);
}

void LoadWindowFont(F_CMD_ARGS)
{
  char *font;
  XFontStruct *newfont;
#ifdef I18N_MB
  XFontSet newfontset;
#endif
#ifdef USEDECOR
  FvwmDecor *fl = cur_decor ? cur_decor : &Scr.DefaultDecor;
#else
  FvwmDecor *fl = &Scr.DefaultDecor;
#endif

  action = GetNextToken(action,&font);
  if (!font)
  {
    /* reset to default font */
    if (Scr.flags.has_window_font)
    {
      XFreeFont(dpy, Scr.DefaultDecor.WindowFont.font);
      Scr.flags.has_window_font = 0;
#ifdef I18N_MB
      fl->WindowFont.fontset = Scr.StdFont.fontset;
      fl->WindowFont.height = Scr.StdFont.height;
#endif
      fl->WindowFont.font = Scr.StdFont.font;
      ApplyWindowFont(&Scr.DefaultDecor);
    }
    return;
  }

#ifdef I18N_MB
  if ((newfontset = GetFontSetOrFixed(dpy, font))!=NULL)
  {
    XFontSetExtents *fset_extents;
    XFontStruct **fs_list;
    char **ml;

    if (fl->WindowFont.fontset != NULL &&
	(fl != &Scr.DefaultDecor || Scr.flags.has_window_font == 1))
      XFreeFontSet(dpy, fl->WindowFont.fontset);
    if (fl == &Scr.DefaultDecor)
      Scr.flags.has_window_font = 1;
    fl->WindowFont.fontset = newfontset;

    /* backward compatiblity setup */
    XFontsOfFontSet(newfontset, &fs_list, &ml);
    fl->WindowFont.font = fs_list[0];
    fset_extents = XExtentsOfFontSet(newfontset);
    fl->WindowFont.height = fset_extents->max_logical_extent.height;
#else
  if ((newfont = GetFontOrFixed(dpy, font)))
  {
    if (fl->WindowFont.font &&
	(fl != &Scr.DefaultDecor || Scr.flags.has_window_font == 1))
      XFreeFont(dpy, fl->WindowFont.font);
    if (fl == &Scr.DefaultDecor)
      Scr.flags.has_window_font = 1;
    fl->WindowFont.font = newfont;
#endif
    ApplyWindowFont(fl);
  }
  else
  {
    fvwm_msg(ERR,"LoadWindowFont","Couldn't load font '%s' or 'fixed'\n",font);
  }

  free(font);
}

void FreeButtonFace(Display *dpy, ButtonFace *bf)
{
    switch (bf->style)
    {
#ifdef GRADIENT_BUTTONS
    case HGradButton:
    case VGradButton:
	/* - should we check visual is not TrueColor before doing this?

	   XFreeColors(dpy, PictureCMap,
		    bf->u.grad.pixels, bf->u.grad.npixels,
		    AllPlanes); */
	free(bf->u.grad.pixels);
	bf->u.grad.pixels = NULL;
	break;
#endif

#ifdef PIXMAP_BUTTONS
    case PixmapButton:
    case TiledPixmapButton:
	if (bf->u.p)
	    DestroyPicture(dpy, bf->u.p);
	bf->u.p = NULL;
	break;
#endif

#ifdef VECTOR_BUTTONS
    case VectorButton:
      if (bf->u.vector.x)
	{
	  free (bf->u.vector.x);
	  bf->u.vector.x = NULL;
	}
      if (bf->u.vector.y)
	{
	  free (bf->u.vector.y);
	  bf->u.vector.y = NULL;
	}
      break;
#endif

    default:
	break;
    }
#ifdef MULTISTYLE
    /* delete any compound styles */
    if (bf->next) {
	FreeButtonFace(dpy, bf->next);
	free(bf->next);
    }
    bf->next = NULL;
#endif
    bf->style = SimpleButton;
}

/*****************************************************************************
 *
 * Reads a button face line into a structure (veliaa@rpi.edu)
 *
 ****************************************************************************/
Boolean ReadButtonFace(char *s, ButtonFace *bf, int button, int verbose)
{
    int offset;
    char style[256], *file;
    char *action = s;

    /* some variants of scanf do not increase the assign count when %n is used,
     * so a return value of 1 is no error. */
    if (sscanf(s, "%256s%n", style, &offset) < 1) {
	if (verbose)
          fvwm_msg(ERR, "ReadButtonFace", "error in face `%s'", s);
	return False;
    }

    if (strncasecmp(style, "--", 2) != 0) {
	s += offset;

	FreeButtonFace(dpy, bf);

	/* determine button style */
	if (strncasecmp(style,"Simple",6)==0)
	{
	    bf->style = SimpleButton;
	}
	else if (strncasecmp(style,"Default",7)==0) {
	    int b = -1, n = sscanf(s, "%d%n", &b, &offset);

	    if (n < 1) {
		if (button == -1) {
		    if(verbose)fvwm_msg(ERR,"ReadButtonFace",
					"need default button number to load");
		    return False;
		}
		b = button;
	    }
	    s += offset;
	    if ((b > 0) && (b <= 10))
		LoadDefaultButton(bf, b);
	    else {
		if(verbose)fvwm_msg(ERR,"ReadButtonFace",
				    "button number out of range: %d", b);
		return False;
	    }
	}
#ifdef VECTOR_BUTTONS
	else if (strncasecmp(style,"Vector",6)==0 ||
		 (strlen(style)<=2 && isdigit(*style)))
	{
	    /* normal coordinate list button style */
	    int i, num_coords, num, line_style;
	    struct vector_coords *vc = &bf->u.vector;

	    /* get number of points */
	    if (strncasecmp(style,"Vector",6)==0) {
		num = sscanf(s,"%d%n",&num_coords,&offset);
		s += offset;
	    } else
		num = sscanf(style,"%d",&num_coords);

	    if((num != 1)||(num_coords>32)||(num_coords<2))
	    {
		if(verbose)fvwm_msg(ERR,"ReadButtonFace",
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
		    if(verbose)fvwm_msg(ERR,"ReadButtonFace",
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
	    bf->style = VectorButton;
	}
#endif
	else if (strncasecmp(style,"Solid",5)==0)
	{
	    s = GetNextToken(s, &file);
	    if (file) {
		bf->style = SolidButton;
		bf->u.back = GetColor(file);
		free(file);
	    } else {
		if(verbose)
		  fvwm_msg(ERR,"ReadButtonFace",
			   "no color given for Solid face type: %s", action);
		return False;
	    }
	}
#ifdef GRADIENT_BUTTONS
	else if (strncasecmp(style,"HGradient",9)==0
		 || strncasecmp(style,"VGradient",9)==0)
	{
	    char *item, **s_colors;
	    int npixels, nsegs, i, sum, *perc;
	    Pixel *pixels;

	    if (!(s = GetNextToken(s, &item)) || (item == NULL)) {
		if(verbose)
		  fvwm_msg(ERR,"ReadButtonFace",
			 "expected number of colors to allocate in gradient");
		if (item)
		  free(item);
		return False;
	    }
	    npixels = atoi(item);
	    free(item);

	    if (!(s = GetNextToken(s, &item)) || (item == NULL)) {
		if(verbose)
		  fvwm_msg(ERR,"ReadButtonFace", "incomplete gradient style");
		if (item)
		  free(item);
		return False;
	    }

	    if (!(isdigit(*item))) {
		s_colors = (char **)safemalloc(sizeof(char *) * 2);
		perc = (int *)safemalloc(sizeof(int));
		nsegs = 1;
		s_colors[0] = item;
		s = GetNextToken(s, &s_colors[1]);
		perc[0] = 100;
	    } else {
		nsegs = atoi(item);
		free(item);
		if (nsegs < 1)
		  nsegs = 1;
		if (nsegs > MAX_GRADIENT_SEGMENTS)
		  nsegs = MAX_GRADIENT_SEGMENTS;
		s_colors = (char **)safemalloc(sizeof(char *) * (nsegs + 1));
		perc = (int *)safemalloc(sizeof(int) * nsegs);
		for (i = 0; i <= nsegs; ++i) {
		    s =GetNextToken(s, &s_colors[i]);
		    if (i < nsegs) {
			s = GetNextToken(s, &item);
			if (item)
			  {
			    perc[i] = atoi(item);
			    free(item);
			  }
			else
			  perc[i] = 0;
		    }
		}
	    }

	    for (i = 0, sum = 0; i < nsegs; ++i)
		sum += perc[i];

	    if (sum != 100) {
		if(verbose)fvwm_msg(ERR,"ReadButtonFace",
				    "multi gradient lengths must sum to 100");
		for (i = 0; i <= nsegs; ++i)
                  if (s_colors[i])
		    free(s_colors[i]);
		free(s_colors);
		free(perc);
		return False;
	    }

	    if (npixels < 2)
	      npixels = 2;
	    if (npixels > MAX_GRADIENT_SEGMENTS)
	      npixels = MAX_GRADIENT_SEGMENTS;

	    pixels = AllocNonlinearGradient(s_colors, perc, nsegs, npixels);
	    for (i = 0; i <= nsegs; ++i)
              if (s_colors[i])
		free(s_colors[i]);
	    free(s_colors);
            free(perc);

	    if (!pixels) {
		if(verbose)fvwm_msg(ERR,"ReadButtonFace",
				    "couldn't create gradient");
		return False;
	    }

	    bf->u.grad.pixels = pixels;
	    bf->u.grad.npixels = npixels;

	    if (strncasecmp(style,"H",1)==0)
		bf->style = HGradButton;
	    else
		bf->style = VGradButton;
	}
#endif /* GRADIENT_BUTTONS */
#ifdef PIXMAP_BUTTONS
	else if (strncasecmp(style,"Pixmap",6)==0
		 || strncasecmp(style,"TiledPixmap",11)==0)
	{
	    s = GetNextToken(s, &file);
	    bf->u.p = CachePicture(dpy, Scr.NoFocusWin, NULL,
				   file,Scr.ColorLimit);
	    if (bf->u.p == NULL)
	    {
	      if (file)
		{
		  if(verbose)fvwm_msg(ERR,"ReadButtonFace",
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

	    if (strncasecmp(style,"Tiled",5)==0)
		bf->style = TiledPixmapButton;
	    else
		bf->style = PixmapButton;
	}
#ifdef MINI_ICONS
	else if (strncasecmp (style, "MiniIcon", 8) == 0) {
	    bf->style = MiniIconButton;
#if 0
/* Have to remove this again. This is all so badly written there is no chance
 * to prevent a coredump and a memory leak the same time without a rewrite of
 * large parts of the code. */
	    if (bf->u.p)
	      DestroyPicture(dpy, bf->u.p);
#endif
	    bf->u.p = NULL; /* pixmap read in when the window is created */
  	}
#endif
#endif /* PIXMAP_BUTTONS */
	else {
	    if(verbose)fvwm_msg(ERR,"ReadButtonFace",
				"unknown style %s: %s", style, action);
	    return False;
	}
    }

    /* Process button flags ("--" signals start of flags,
       it is also checked for above) */
    s = GetNextToken(s, &file);
    if (file && (strcmp(file,"--")==0)) {
	char *tok;
	s = GetNextToken(s, &tok);
	while (tok&&*tok)
	{
	    int set = 1;

	    if (*tok == '!') { /* flag negate */
		set = 0;
		++tok;
	    }
	    if (StrEquals(tok,"Clear")) {
		if (set)
		    bf->style &= ButtonFaceTypeMask;
		else
		    bf->style |= ~ButtonFaceTypeMask; /* ? */
	    }
	    else if (StrEquals(tok,"Left"))
	    {
		if (set) {
		    bf->style |= HOffCenter;
		    bf->style &= ~HRight;
		} else
		    bf->style |= HOffCenter | HRight;
	    }
	    else if (StrEquals(tok,"Right"))
	    {
		if (set)
		    bf->style |= HOffCenter | HRight;
		else {
		    bf->style |= HOffCenter;
		    bf->style &= ~HRight;
		}
	    }
	    else if (StrEquals(tok,"Centered")) {
		bf->style &= ~HOffCenter;
		bf->style &= ~VOffCenter;
	    }
	    else if (StrEquals(tok,"Top"))
	    {
		if (set) {
		    bf->style |= VOffCenter;
		    bf->style &= ~VBottom;
		} else
		    bf->style |= VOffCenter | VBottom;

	    }
	    else if (StrEquals(tok,"Bottom"))
	    {
		if (set)
		    bf->style |= VOffCenter | VBottom;
		else {
		    bf->style |= VOffCenter;
		    bf->style &= ~VBottom;
		}
	    }
	    else if (StrEquals(tok,"Flat"))
	    {
		if (set) {
		    bf->style &= ~SunkButton;
		    bf->style |= FlatButton;
		} else
		    bf->style &= ~FlatButton;
	    }
	    else if (StrEquals(tok,"Sunk"))
	    {
		if (set) {
		    bf->style &= ~FlatButton;
		    bf->style |= SunkButton;
		} else
		    bf->style &= ~SunkButton;
	    }
	    else if (StrEquals(tok,"Raised"))
	    {
		if (set) {
		    bf->style &= ~FlatButton;
		    bf->style &= ~SunkButton;
		} else {
		    bf->style |= SunkButton;
		    bf->style &= ~FlatButton;
		}
	    }
#ifdef EXTENDED_TITLESTYLE
	    else if (StrEquals(tok,"UseTitleStyle"))
	    {
		if (set) {
		    bf->style |= UseTitleStyle;
#ifdef BORDERSTYLE
		    bf->style &= ~UseBorderStyle;
#endif
		} else
		    bf->style &= ~UseTitleStyle;
	    }
#endif
#ifdef BORDERSTYLE
	    else if (StrEquals(tok,"HiddenHandles"))
	    {
		if (set)
		    bf->style |= HiddenHandles;
		else
		    bf->style &= ~HiddenHandles;
	    }
	    else if (StrEquals(tok,"NoInset"))
	    {
		if (set)
		    bf->style |= NoInset;
		else
		    bf->style &= ~NoInset;
	    }
	    else if (StrEquals(tok,"UseBorderStyle"))
	    {
		if (set) {
		    bf->style |= UseBorderStyle;
#ifdef EXTENDED_TITLESTYLE
		    bf->style &= ~UseTitleStyle;
#endif
		} else
		    bf->style &= ~UseBorderStyle;
	    }
#endif
	    else
		if(verbose)
		  fvwm_msg(ERR,"ReadButtonFace",
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
char *ReadTitleButton(char *s, TitleButton *tb, Boolean append, int button)
{
    char *end = NULL, *spec;
    ButtonFace tmpbf;
    enum ButtonState bs = MaxButtonState;
    int i = 0, all = 0, pstyle = 0;

    while(isspace((unsigned char)*s))++s;
    for (; i < MaxButtonState; ++i)
	if (strncasecmp(button_states[i],s,
			  strlen(button_states[i]))==0) {
	    bs = i;
	    break;
	}
    if (bs != MaxButtonState)
	s += strlen(button_states[bs]);
    else
	all = 1;
    while(isspace((unsigned char)*s))++s;
    if ('(' == *s) {
	int len;
	pstyle = 1;
	if (!(end = strchr(++s, ')'))) {
	    fvwm_msg(ERR,"ReadTitleButton",
		     "missing parenthesis: %s", s);
	    return NULL;
	}
	while (isspace((unsigned char)*s)) ++s;
	len = end - s + 1;
	spec = safemalloc(len);
	strncpy(spec, s, len - 1);
	spec[len - 1] = 0;
    } else
	spec = s;

    while(isspace((unsigned char)*spec))++spec;
    /* setup temporary in case button read fails */
    tmpbf.style = SimpleButton;
#ifdef MULTISTYLE
    tmpbf.next = NULL;
#endif
#ifdef MINI_ICONS
#ifdef PIXMAP_BUTTONS
    tmpbf.u.p = NULL;
#endif
#endif

    if (strncmp(spec, "--",2)==0) {
	/* only change flags */
	if (ReadButtonFace(spec, &tb->state[all ? 0 : bs],button,True) && all) {
	    for (i = 0; i < MaxButtonState; ++i)
		ReadButtonFace(spec, &tb->state[i],-1,False);
	}
    }
    else if (ReadButtonFace(spec, &tmpbf,button,True)) {
	int b = all ? 0 : bs;
#ifdef MULTISTYLE
	if (append) {
	    ButtonFace *tail = &tb->state[b];
	    while (tail->next) tail = tail->next;
	    tail->next = (ButtonFace *)safemalloc(sizeof(ButtonFace));
	    *tail->next = tmpbf;
	    if (all)
		for (i = 1; i < MaxButtonState; ++i) {
		    tail = &tb->state[i];
		    while (tail->next) tail = tail->next;
		    tail->next = (ButtonFace *)safemalloc(sizeof(ButtonFace));
		    tail->next->style = SimpleButton;
		    tail->next->next = NULL;
		    ReadButtonFace(spec, tail->next, button, False);
		}
	}
	else {
#endif
	    FreeButtonFace(dpy, &tb->state[b]);
	    tb->state[b] = tmpbf;
	    if (all)
		for (i = 1; i < MaxButtonState; ++i)
		    ReadButtonFace(spec, &tb->state[i],button,False);
#ifdef MULTISTYLE
	}
#endif

    }
    if (pstyle) {
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
void AddToDecor(FvwmDecor *fl, char *s)
{
  if (!s)
    return;
  while (*s&&isspace((unsigned char)*s))
    ++s;
  if (!*s)
    return;
  cur_decor = fl;
  ExecuteFunction(s,NULL,&Event,C_ROOT,-1,EXPAND_COMMAND);
  cur_decor = NULL;
}

/*****************************************************************************
 *
 * Changes the window's FvwmDecor pointer (veliaa@rpi.edu)
 *
 ****************************************************************************/
void ChangeDecor(F_CMD_ARGS)
{
    char *item;
    int x,y,width,height,old_height,extra_height;
    FvwmDecor *fl = &Scr.DefaultDecor, *found = NULL;
    if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
	return;
    action = GetNextToken(action, &item);
    if (!action || !item)
      {
	if (item)
	  free(item);
	return;
      }
    /* search for tag */
    for (; fl; fl = fl->next)
	if (fl->tag)
	    if (StrEquals(item, fl->tag)) {
		found = fl;
		break;
	    }
    free(item);
    if (!found) {
	XBell(dpy, 0);
	return;
    }
    old_height = tmp_win->fl->TitleHeight;
    tmp_win->fl = found;
    extra_height = (HAS_TITLE(tmp_win)) ?
      (old_height - tmp_win->fl->TitleHeight) : 0;
    x = tmp_win->frame_g.x;
    y = tmp_win->frame_g.y;
    width = tmp_win->frame_g.width;
    height = tmp_win->frame_g.height - extra_height;
    tmp_win->frame_g.x = 0;
    tmp_win->frame_g.y = 0;
    tmp_win->frame_g.height = 0;
    tmp_win->frame_g.width = 0;
    SetupFrame(tmp_win,x,y,width,height,True,False);
    SetBorder(tmp_win,Scr.Hilite == tmp_win,2,True,None);
}

/*****************************************************************************
 *
 * Destroys an FvwmDecor (veliaa@rpi.edu)
 *
 ****************************************************************************/
void DestroyDecor(F_CMD_ARGS)
{
    char *item;
    FvwmDecor *fl = Scr.DefaultDecor.next;
    FvwmDecor *prev = &Scr.DefaultDecor, *found = NULL;

    action = GetNextToken(action, &item);
    if (!action || !item)
      {
	if (item)
	  free(item);
	return;
      }

    /* search for tag */
    for (; fl; fl = fl->next) {
	if (fl->tag)
	    if (StrEquals(item, fl->tag)) {
		found = fl;
		break;
	    }
	prev = fl;
    }
    free(item);

    if (found && (found != &Scr.DefaultDecor)) {
	FvwmWindow *fw = Scr.FvwmRoot.next;
	while(fw)
	{
	    if (fw->fl == found)
	      ExecuteFunction("ChangeDecor Default",fw,eventp,
			      C_WINDOW,*Module,EXPAND_COMMAND);
	    fw = fw->next;
	}
	prev->next = found->next;
	DestroyFvwmDecor(found);
	free(found);
    }
}

/*****************************************************************************
 *
 * Initiates an AddToDecor (veliaa@rpi.edu)
 *
 ****************************************************************************/
void add_item_to_decor(F_CMD_ARGS)
{
    FvwmDecor *fl, *found = NULL;
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
    for (fl = &Scr.DefaultDecor; fl; fl = fl->next)
	if (fl->tag)
	    if (StrEquals(item, fl->tag)) {
		found = fl;
		break;
	    }
    if (!found) { /* then make a new one */
	found = (FvwmDecor *)safemalloc(sizeof( FvwmDecor ));
	InitFvwmDecor(found);
	found->tag = item; /* tag it */
	/* add it to list */
	for (fl = &Scr.DefaultDecor; fl->next; fl = fl->next)
	  ;
	fl->next = found;
    } else
	free(item);

    if (found) {
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
    FvwmDecor *fl = &Scr.DefaultDecor, *found = NULL;
    char *item = NULL;
    action = GetNextToken(action, &item);
    if (item) {
	/* search for tag */
	for (; fl; fl = fl->next)
	    if (fl->tag)
		if (strcasecmp(item, fl->tag)==0) {
		    found = fl;
		    break;
		}
	free(item);
    }
#endif

    for (; fw; fw = fw->next)
    {
#ifdef USEDECOR
	/* update specific decor, or all */
	if (found) {
	    if (fw->fl == found) {
		SetBorder(fw,True,True,True,None);
		SetBorder(fw,False,True,True,None);
	    }
	}
	else
#endif
	{
	    SetBorder(fw,True,True,True,None);
	    SetBorder(fw,False,True,True,None);
	}
    }
    SetBorder(Scr.Hilite,True,True,True,None);
}


/*****************************************************************************
 *
 * Changes a button decoration style (changes by veliaa@rpi.edu)
 *
 ****************************************************************************/
#define SetButtonFlag(flag)	do {				\
    int i;							\
								\
    if (multi) {						\
	if (multi & 1)						\
	    for (i = 0; i < 5; ++i) {				\
		if (set)					\
		    fl->left_buttons[i].flags |= (flag);	\
		else						\
		    fl->left_buttons[i].flags &= ~(flag);	\
	    }							\
	if (multi & 2)						\
	    for (i = 0; i < 5; ++i) {				\
		if (set)					\
		    fl->right_buttons[i].flags |= (flag);	\
		else						\
		    fl->right_buttons[i].flags &= ~(flag);	\
	    }							\
    } else {							\
	if (set)						\
	    tb->flags |= (flag);				\
	else							\
	    tb->flags &= ~(flag);				\
    }								\
} while (0)

void ButtonStyle(F_CMD_ARGS)
{
    int button = 0,n;
    int multi = 0;
    char *text = action, *prev;
    char *parm = NULL;
    TitleButton *tb = NULL;
#ifdef USEDECOR
    FvwmDecor *fl = cur_decor ? cur_decor : &Scr.DefaultDecor;
#else
    FvwmDecor *fl = &Scr.DefaultDecor;
#endif

    text = GetNextToken(text, &parm);
    if (parm && isdigit(*parm))
	button = atoi(parm);

    if ((parm == NULL) || (button > 10) || (button < 0)) {
	fvwm_msg(ERR,"ButtonStyle","Bad button style (1) in line %s",action);
	if (parm)
	  free(parm);
	return;
    }

    if (!isdigit(*parm)) {
	if (StrEquals(parm,"left"))
	    multi = 1; /* affect all left buttons */
	else if (StrEquals(parm,"right"))
	    multi = 2; /* affect all right buttons */
	else if (StrEquals(parm,"all"))
	    multi = 3; /* affect all buttons */
	else {
	    /* we're either resetting buttons or
	       an invalid button set was specified */
	    if (StrEquals(parm,"reset"))
		ResetAllButtons(fl);
	    else
		fvwm_msg(ERR,"ButtonStyle","Bad button style (2) in line %s",
			 action);
	    free(parm);
	    return;
	}
    }
    free(parm);
    if (multi == 0) {
	/* a single button was specified */
	if (button==10) button=0;
	/* which arrays to use? */
	n=button/2;
	if((n*2) == button)
	{
	    /* right */
	    n = n - 1;
	    if(n<0)n=4;
	    tb = &fl->right_buttons[n];
	}
	else {
	    /* left */
	    tb = &fl->left_buttons[n];
	}
    }

    prev = text;
    text = GetNextToken(text,&parm);
    while(parm)
    {
	if (strcmp(parm,"-")==0) {
	    char *tok;
	    text = GetNextToken(text, &tok);
	    while (tok)
	    {
		int set = 1;

		if (*tok == '!') { /* flag negate */
		    set = 0;
		    ++tok;
		}
		if (StrEquals(tok,"Clear")) {
		    int i;
		    if (multi) {
			if (multi&1) {
			    for (i=0;i<5;++i)
				if (set)
				    fl->left_buttons[i].flags = 0;
				else
				    fl->left_buttons[i].flags = ~0;
			}
			if (multi&2) {
			    for (i=0;i<5;++i) {
				if (set)
				    fl->right_buttons[i].flags = 0;
				else
				    fl->right_buttons[i].flags = ~0;
			    }
			}
		    } else {
			if (set)
			    tb->flags = 0;
			else
			    tb->flags = ~0;
		    }
		}
                else if (strncasecmp(tok,"MWMDecorMenu",12)==0) {
		  SetButtonFlag(MWMDecorMenu);
		} else if (strncasecmp(tok,"MWMDecorMin",11)==0) {
		  SetButtonFlag(MWMDecorMinimize);
                } else if (strncasecmp(tok,"MWMDecorMax",11)==0) {
		  SetButtonFlag(MWMDecorMaximize);
                } else if (strncasecmp(tok,"MWMDecorShade",13)==0) {
		  SetButtonFlag(MWMDecorShade);
                } else if (strncasecmp(tok,"MWMDecorStick",13)==0) {
		  SetButtonFlag(MWMDecorStick);
		} else {
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
	    free(parm);
	    break;
	} else {
	    if (multi) {
		int i;
		if (multi&1)
		    for (i=0;i<5;++i)
			text = ReadTitleButton(prev, &fl->left_buttons[i],
					       False, i*2+1);
		if (multi&2)
		    for (i=0;i<5;++i)
			text = ReadTitleButton(prev, &fl->right_buttons[i],
					       False, i*2);
	    }
	    else if (!(text = ReadTitleButton(prev, tb, False, button))) {
		free(parm);
		break;
	    }
	}
	free(parm);
	prev = text;
	text = GetNextToken(text,&parm);
    }
}

#ifdef MULTISTYLE
/*****************************************************************************
 *
 * Appends a button decoration style (veliaa@rpi.edu)
 *
 ****************************************************************************/
void AddButtonStyle(F_CMD_ARGS)
{
    int button = 0,n;
    int multi = 0;
    char *text = action, *prev;
    char *parm = NULL;
    TitleButton *tb = NULL;
#ifdef USEDECOR
    FvwmDecor *fl = cur_decor ? cur_decor : &Scr.DefaultDecor;
#else
    FvwmDecor *fl = &Scr.DefaultDecor;
#endif

    text = GetNextToken(text, &parm);
    if (parm && isdigit(*parm))
	button = atoi(parm);

    if ((parm == NULL) || (button > 10) || (button < 0)) {
	fvwm_msg(ERR,"ButtonStyle","Bad button style (1) in line %s",action);
	if (parm)
	  free(parm);
	return;
    }

    if (!isdigit(*parm)) {
	if (StrEquals(parm,"left"))
	    multi = 1; /* affect all left buttons */
	else if (StrEquals(parm,"right"))
	    multi = 2; /* affect all right buttons */
	else if (StrEquals(parm,"all"))
	    multi = 3; /* affect all buttons */
	else {
	    /* we're either resetting buttons or
	       an invalid button set was specified */
	    if (StrEquals(parm,"reset"))
		ResetAllButtons(fl);
	    else
		fvwm_msg(ERR,"ButtonStyle","Bad button style (2) in line %s",
			 action);
	    free(parm);
	    return;
	}
    }
    free(parm);
    if (multi == 0) {
	/* a single button was specified */
	if (button==10) button=0;
	/* which arrays to use? */
	n=button/2;
	if((n*2) == button)
	{
	    /* right */
	    n = n - 1;
	    if(n<0)n=4;
	    tb = &fl->right_buttons[n];
	}
	else {
	    /* left */
	    tb = &fl->left_buttons[n];
	}
    }

    prev = text;
    text = GetNextToken(text,&parm);
    while(parm)
    {
	if (multi) {
	    int i;
	    if (multi&1)
		for (i=0;i<5;++i)
		    text = ReadTitleButton(prev, &fl->left_buttons[i], True,
					   i*2+1);
	    if (multi&2)
		for (i=0;i<5;++i)
		    text = ReadTitleButton(prev, &fl->right_buttons[i], True,
					   i*2);
	}
	else if (!(text = ReadTitleButton(prev, tb, True, button))) {
	    free(parm);
	    break;
	}
	free(parm);
	prev = text;
	text = GetNextToken(text,&parm);
    }
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

/**********************************************************************
 * Parses the flag string and returns the text between [ ] or ( )
 * characters.  The start of the rest of the line is put in restptr.
 * Note that the returned string is allocated here and it must be
 * freed when it is not needed anymore.
 **********************************************************************/
static char *CreateFlagString(char *string, char **restptr)
{
  char *retval;
  char *c;
  char *start;
  char closeopt;
  int length;

  c = string;
  while (isspace((unsigned char)*c) && (*c != 0))
    c++;

  if (*c == '[' || *c == '(')
  {
    /* Get the text between [ ] or ( ) */
    if (*c == '[')
      closeopt = ']';
    else
      closeopt = ')';
    c++;
    start = c;
    length = 0;
    while (*c != closeopt) {
      if (*c == 0) {
	fvwm_msg(ERR, "CreateFlagString",
		 "Conditionals require closing parenthesis");
	*restptr = NULL;
	return NULL;
      }
      c++;
      length++;
    }

    /* We must allocate a new string because we null terminate the string
     * between the [ ] or ( ) characters.
     */
    retval = safemalloc(length + 1);
    strncpy(retval, start, length);
    retval[length] = 0;

    *restptr = c + 1;
  }
  else {
    retval = NULL;
    *restptr = c;
  }

  return retval;
}

/**********************************************************************
 * The name field of the mask is allocated in CreateConditionMask.
 * It must be freed.
 **********************************************************************/
static void FreeConditionMask(WindowConditionMask *mask)
{
  if (mask->my_flags.needs_name)
    free(mask->name);
  else if (mask->my_flags.needs_not_name)
    free(mask->name - 1);
}

/* Assign the default values for the window mask */
static void DefaultConditionMask(WindowConditionMask *mask)
{
  memset(mask, 0, sizeof(WindowConditionMask));
  mask->layer = -2; /* -2  means no layer condition, -1 means current */
}

/**********************************************************************
 * Note that this function allocates the name field of the mask struct.
 * FreeConditionMask must be called for the mask when the mask is discarded.
 **********************************************************************/
static void CreateConditionMask(char *flags, WindowConditionMask *mask)
{
  char *condition;
  char *prev_condition = NULL;
  char *tmp;

  if (flags == NULL)
    return;

  /* Next parse the flags in the string. */
  tmp = flags;
  tmp = GetNextToken(tmp, &condition);

  while (condition)
  {
    if (StrEquals(condition,"Iconic"))
      {
	SET_ICONIFIED(mask, 1);
	SETM_ICONIFIED(mask, 1);
      }
    else if(StrEquals(condition,"!Iconic"))
      {
	SET_ICONIFIED(mask, 0);
	SETM_ICONIFIED(mask, 1);
      }
    else if(StrEquals(condition,"Visible"))
      {
	SET_PARTIALLY_VISIBLE(mask, 1);
	SETM_PARTIALLY_VISIBLE(mask, 1);
      }
    else if(StrEquals(condition,"!Visible"))
      {
	SET_PARTIALLY_VISIBLE(mask, 0);
	SETM_PARTIALLY_VISIBLE(mask, 1);
      }
    else if(StrEquals(condition,"Raised"))
      {
	SET_FULLY_VISIBLE(mask, 1);
	SETM_FULLY_VISIBLE(mask, 1);
      }
    else if(StrEquals(condition,"!Raised"))
      {
	SET_FULLY_VISIBLE(mask, 0);
	SETM_FULLY_VISIBLE(mask, 1);
      }
    else if(StrEquals(condition,"Sticky"))
      {
	SET_STICKY(mask, 1);
	SETM_STICKY(mask, 1);
      }
    else if(StrEquals(condition,"!Sticky"))
      {
	SET_STICKY(mask, 0);
	SETM_STICKY(mask, 1);
      }
    else if(StrEquals(condition,"Maximized"))
      {
	SET_MAXIMIZED(mask, 1);
	SETM_MAXIMIZED(mask, 1);
      }
    else if(StrEquals(condition,"!Maximized"))
      {
	SET_MAXIMIZED(mask, 0);
	SETM_MAXIMIZED(mask, 1);
      }
    else if(StrEquals(condition,"Shaded"))
      {
	SET_SHADED(mask, 1);
	SETM_SHADED(mask, 1);
      }
    else if(StrEquals(condition,"!Shaded"))
      {
	SET_SHADED(mask, 0);
	SETM_SHADED(mask, 1);
      }
    else if(StrEquals(condition,"Transient"))
      {
	SET_TRANSIENT(mask, 1);
	SETM_TRANSIENT(mask, 1);
      }
    else if(StrEquals(condition,"!Transient"))
      {
	SET_TRANSIENT(mask, 0);
	SETM_TRANSIENT(mask, 1);
      }
    else if(StrEquals(condition,"CurrentDesk"))
      mask->my_flags.needs_current_desk = 1;
    else if(StrEquals(condition,"CurrentPage"))
    {
      mask->my_flags.needs_current_desk = 1;
      mask->my_flags.needs_current_page = 1;
    }
    else if(StrEquals(condition,"CurrentPageAnyDesk") ||
	    StrEquals(condition,"CurrentScreen"))
      mask->my_flags.needs_current_page = 1;
    else if(StrEquals(condition,"CirculateHit"))
      mask->my_flags.use_circulate_hit = 1;
    else if(StrEquals(condition,"CirculateHitIcon"))
      mask->my_flags.use_circulate_hit_icon = 1;
    else if (StrEquals(condition, "Layer"))
    {
       if (sscanf(tmp,"%d",&mask->layer))
       {
	  tmp = GetNextToken (tmp, &condition);
          if (mask->layer < 0)
            mask->layer = -2; /* silently ignore invalid layers */
       }
       else
       {
          mask->layer = -1; /* needs current layer */
       }
    }
    else if(!mask->my_flags.needs_name && !mask->my_flags.needs_not_name)
    {
      /* only 1st name to avoid mem leak */
      mask->name = condition;
      condition = NULL;
      if (mask->name[0] == '!')
      {
	mask->my_flags.needs_not_name = 1;
	mask->name++;
      }
      else
	mask->my_flags.needs_name = 1;
    }

    if (prev_condition)
      free(prev_condition);

    prev_condition = condition;
    tmp = GetNextToken(tmp, &condition);
  }

  if(prev_condition)
    free(prev_condition);
}

/**********************************************************************
 * Checks whether the given window matches the mask created with
 * CreateConditionMask.
 **********************************************************************/
static Bool MatchesConditionMask(FvwmWindow *fw, WindowConditionMask *mask)
{
  Bool fMatchesName;
  Bool fMatchesIconName;
  Bool fMatchesClass;
  Bool fMatchesResource;
  Bool fMatches;

  if (cmp_masked_flags(&(fw->flags), &(mask->flags),
		       &(mask->flag_mask), sizeof(fw->flags)) != 0)
    return 0;

  /*
  if ((mask->onFlags & fw->flags) != mask->onFlags)
    return 0;

  if ((mask->offFlags & fw->flags) != 0)
    return 0;
  */

  if (!mask->my_flags.use_circulate_hit && DO_SKIP_CIRCULATE(fw))
    return 0;

  /* This logic looks terribly wrong to me, but it was this way before so I
   * did not change it (domivogt (24-Dec-1998)) */
  if (!DO_SKIP_ICON_CIRCULATE(mask) && IS_ICONIFIED(fw) &&
      DO_SKIP_ICON_CIRCULATE(fw))
    return 0;

  if (IS_ICONIFIED(fw) && IS_TRANSIENT(fw) && IS_ICONIFIED_BY_PARENT(fw))
    return 0;

  if (mask->my_flags.needs_current_desk && fw->Desk != Scr.CurrentDesk)
    return 0;

  if (mask->my_flags.needs_current_page &&
      !(fw->frame_g.x < Scr.MyDisplayWidth &&
	fw->frame_g.y < Scr.MyDisplayHeight &&
	fw->frame_g.x + fw->frame_g.width > 0 &&
	fw->frame_g.y + fw->frame_g.height > 0))
    return 0;

  /* Yes, I know this could be shorter, but it's hard to understand then */
  fMatchesName = matchWildcards(mask->name, fw->name);
  fMatchesIconName = matchWildcards(mask->name, fw->icon_name);
  fMatchesClass = (fw->class.res_class &&
		   matchWildcards(mask->name,fw->class.res_class));
  fMatchesResource = (fw->class.res_name &&
		      matchWildcards(mask->name, fw->class.res_name));
  fMatches = (fMatchesName || fMatchesIconName || fMatchesClass ||
	      fMatchesResource);

  if (mask->my_flags.needs_name && !fMatches)
    return 0;

  if (mask->my_flags.needs_not_name && fMatches)
    return 0;

  if ((mask->layer == -1) && (fw->layer != Scr.Focus->layer))
    return 0;

  if ((mask->layer >= 0) && (fw->layer != mask->layer))
    return 0;

  return 1;
}

/**************************************************************************
 *
 * Direction = 1 ==> "Next" operation
 * Direction = -1 ==> "Previous" operation
 * Direction = 0 ==> operation on current window (returns pass or fail)
 *
 **************************************************************************/
static FvwmWindow *Circulate(char *action, int Direction, char **restofline)
{
  int pass = 0;
  FvwmWindow *fw, *found = NULL;
  WindowConditionMask mask;
  char *flags;

  /* Create window mask */
  flags = CreateFlagString(action, restofline);
  DefaultConditionMask(&mask);
  if (Direction == 0) { /* override for Current [] */
    mask.my_flags.use_circulate_hit = 1;
    mask.my_flags.use_circulate_hit_icon = 1;
  }
  CreateConditionMask(flags, &mask);
  if (flags)
    free(flags);

  if(Scr.Focus)
  {
    if(Direction == 1)
      fw = Scr.Focus->prev;
    else if(Direction == -1)
      fw = Scr.Focus->next;
    else
      fw = Scr.Focus;
  }
  else
    fw = NULL;

  while((pass < 3)&&(found == NULL))
  {
    while((fw)&&(found==NULL)&&(fw != &Scr.FvwmRoot))
    {
#ifdef FVWM_DEBUG_MSGS
      fvwm_msg(DBG,"Circulate","Trying %s",fw->name);
#endif /* FVWM_DEBUG_MSGS */
      /* Make CirculateUp and CirculateDown take args. by Y.NOMURA */
      if (MatchesConditionMask(fw, &mask))
	found = fw;
      else
      {
	if(Direction == 1)
	  fw = fw->prev;
	else
	  fw = fw->next;
      }
      if (Direction == 0)
      {
	FreeConditionMask(&mask);
        return found;
      }
    }
    if((fw == NULL)||(fw == &Scr.FvwmRoot))
    {
      if(Direction == 1)
      {
        /* Go to end of list */
        fw = &Scr.FvwmRoot;
        while(fw && fw->next)
        {
          fw = fw->next;
        }
      }
      else
      {
        /* GO to top of list */
        fw = Scr.FvwmRoot.next;
      }
    }
    pass++;
  }
  FreeConditionMask(&mask);
  return found;
}

void PrevFunc(F_CMD_ARGS)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, -1, &restofline);
  if(found && restofline)
  {
    ExecuteFunction(restofline,found,eventp,C_WINDOW,*Module,EXPAND_COMMAND);
  }

}

void NextFunc(F_CMD_ARGS)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, 1, &restofline);
  if(found && restofline)
  {
    ExecuteFunction(restofline,found,eventp,C_WINDOW,*Module,EXPAND_COMMAND);
  }

}

void NoneFunc(F_CMD_ARGS)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, 1, &restofline);
  if(!found && restofline)
  {
    ExecuteFunction(restofline,NULL,eventp,C_ROOT,*Module,EXPAND_COMMAND);
  }
}

void CurrentFunc(F_CMD_ARGS)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, 0, &restofline);
  if(found && restofline)
  {
    ExecuteFunction(restofline,found,eventp,C_WINDOW,*Module,EXPAND_COMMAND);
  }
}

void AllFunc(F_CMD_ARGS)
{
  FvwmWindow *t, **g;
  char *restofline;
  WindowConditionMask mask;
  char *flags;
  int num, i;

  flags = CreateFlagString(action, &restofline);
  DefaultConditionMask(&mask);
  CreateConditionMask(flags, &mask);
  mask.my_flags.use_circulate_hit = 1;
  mask.my_flags.use_circulate_hit_icon = 1;

  num = 0;
  for (t = Scr.FvwmRoot.next; t; t = t->next)
    {
       num++;
    }

  g = (FvwmWindow **) malloc (num * sizeof(FvwmWindow *));

  num = 0;
  for (t = Scr.FvwmRoot.next; t; t = t->next)
    {
      if (MatchesConditionMask(t, &mask))
	{
          g[num++] = t;
        }
    }

  for (i = 0; i < num; i++)
    {
       ExecuteFunction(restofline,g[i],eventp,C_WINDOW,*Module,EXPAND_COMMAND);
    }

  free (g);
  FreeConditionMask(&mask);
}

static void GetDirectionReference(FvwmWindow *w, int *x, int *y)
{
  if (IS_ICONIFIED(w))
  {
    *x = w->icon_x_loc + w->icon_w_width / 2;
    *y = w->icon_y_loc + w->icon_w_height / 2;
  }
  else
  {
    *x = w->frame_g.x + w->frame_g.width / 2;
    *y = w->frame_g.y + w->frame_g.height / 2;
  }
}

/**********************************************************************
 * Execute a function to the closest window in the given
 * direction.
 **********************************************************************/
void DirectionFunc(F_CMD_ARGS)
{
  char *directions[] = { "North", "East", "South", "West", "NorthEast",
			 "SouthEast", "SouthWest", "NorthWest", NULL };
  int my_x;
  int my_y;
  int his_x;
  int his_y;
  int score;
  int offset = 0;
  int distance = 0;
  int best_score;
  FvwmWindow *window;
  FvwmWindow *best_window;
  int dir;
  char *flags;
  char *restofline;
  char *tmp;
  WindowConditionMask mask;

  /* Parse the direction. */
  action = GetNextToken(action, &tmp);
  dir = GetTokenIndex(tmp, directions, 0, NULL);
  if (dir == -1)
  {
    fvwm_msg(ERR, "Direction","Invalid direction %s", (tmp)? tmp : "");
    if (tmp)
      free(tmp);
    return;
  }
  if (tmp)
    free(tmp);

  /* Create the mask for flags */
  flags = CreateFlagString(action, &restofline);
  if (!restofline)
  {
    if (flags)
      free(flags);
    return;
  }
  DefaultConditionMask(&mask);
  CreateConditionMask(flags, &mask);
  if (flags)
    free(flags);

  /* If there is a focused window, use that as a starting point.
   * Otherwise we use the pointer as a starting point. */
  if (tmp_win)
  {
    GetDirectionReference(tmp_win, &my_x, &my_y);
  }
  else
    XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		  &my_x, &my_y, &JunkX, &JunkY, &JunkMask);

  /* Next we iterate through all windows and choose the closest one in
   * the wanted direction.
   */
  best_window = NULL;
  best_score = -1;
  for (window = Scr.FvwmRoot.next; window; window = window->next) {
    /* Skip every window that does not match conditionals.
     * Skip also currently focused window.  That would be too close. :)
     */
    if (window == tmp_win || !MatchesConditionMask(window, &mask))
      continue;

    /* Calculate relative location of the window. */
    GetDirectionReference(window, &his_x, &his_y);
    his_x -= my_x;
    his_y -= my_y;

    if (dir > 3)
    {
      int tx;
      /* Rotate the diagonals 45 degrees counterclockwise. To do this,
       * multiply the matrix /+h +h\ with the vector (x y).
       *                     \-h +h/
       * h = sqrt(0.5). We can set h := 1 since absolute distance doesn't
       * matter here. */
      tx = his_x + his_y;
      his_y = -his_x + his_y;
      his_x = tx;
    }
    /* Arrange so that distance and offset are positive in desired direction.
     */
    switch (dir)
    {
    case 0: /* N */
    case 2: /* S */
    case 4: /* NE */
    case 6: /* SW */
      offset = (his_x < 0) ? -his_x : his_x;
      distance = (dir == 0 || dir == 4) ? -his_y : his_y;
      break;
    case 1: /* E */
    case 3: /* W */
    case 5: /* SE */
    case 7: /* NW */
      offset = (his_y < 0) ? -his_y : his_y;
      distance = (dir == 3 || dir == 7) ? -his_x : his_x;
      break;
    }

    /* Target must be in given direction. */
    if (distance <= 0) continue;

    /* Calculate score for this window.  The smaller the better. */
    score = 1024 * offset / distance + 2 * distance + 2 * offset;
    if (best_score == -1 || score < best_score) {
      best_window = window;
      best_score = score;
    }
  } /* for */

  if (best_window)
    ExecuteFunction(restofline, best_window, eventp, C_WINDOW, *Module,
		    EXPAND_COMMAND);

  FreeConditionMask(&mask);
}

/* A very simple function, but handy if you want to call
 * complex functions from root context without selecting a window
 * for every single function in it. */
void PickFunc(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
    return;

  ExecuteFunction(action, tmp_win, eventp, C_WINDOW, *Module, EXPAND_COMMAND);
}

void WindowIdFunc(F_CMD_ARGS)
{
  FvwmWindow *t;
  char *num;
  unsigned long win;
  Bool use_condition = False;
  WindowConditionMask mask;
  char *flags, *restofline;

  /* Get window ID */
  action = GetNextToken(action, &num);

  if (num)
    {
      win = (unsigned long)strtol(num,NULL,0); /* SunOS doesn't have strtoul */
      free(num);
    }
  else
    win = 0;

  /* Look for condition - CreateFlagString returns NULL if no '(' or '[' */
  flags = CreateFlagString(action, &restofline);
  if (flags)
    {
    /* Create window mask */
    use_condition = True;
    DefaultConditionMask(&mask);

    /* override for Current [] */
    mask.my_flags.use_circulate_hit = 1;
    mask.my_flags.use_circulate_hit_icon = 1;

    CreateConditionMask(flags, &mask);
    free(flags);

    /* Relocate action */
    action = restofline;
    }

  /* Search windows */
  for (t = Scr.FvwmRoot.next; t; t = t->next)
    if (t->w == win) {
      /* do it if no conditions or the conditions match */
      if (!use_condition || MatchesConditionMask(t, &mask))
        ExecuteFunction(action,t,eventp,C_WINDOW,*Module,EXPAND_COMMAND);
      break;
  }

  /* Tidy up */
  if (use_condition)
  {
    FreeConditionMask(&mask);
  }
}


/***********************************************************************
 *
 *  Procedure:
 *	Reborder - Removes fvwm border windows
 *
 ************************************************************************/
static void do_recapture(F_CMD_ARGS, Bool fSingle)
{
  XEvent event;

  if (fSingle)
    if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
      return;

  /* Wow, this grabbing speeds up recapture tremendously! I think that is the
   * solution for this weird -blackout option. */
  MyXGrabServer(dpy);
  GrabEm(CRS_WAIT);
  XSync(dpy,0);
  if (fSingle)
    CaptureOneWindow(tmp_win, tmp_win->w);
  else
    CaptureAllWindows();
  /* Throw away queued up events. We don't want user input during a
   * recapture. The window the user clicks in might disapper at the very same
   * moment and the click goes through to the root window. Not goot */
  while (XCheckMaskEvent(dpy, ButtonPressMask|ButtonReleaseMask|
			 ButtonMotionMask|PointerMotionMask|EnterWindowMask|
			 LeaveWindowMask|KeyPressMask|KeyReleaseMask,
			 &event) != False)
    ;
  UngrabEm();
  MyXUngrabServer(dpy);
  XSync(dpy, 0);
}

void Recapture(F_CMD_ARGS)
{
  do_recapture(eventp, w, tmp_win, context, action, Module, False);
}

void RecaptureWindow(F_CMD_ARGS)
{
  do_recapture(eventp, w, tmp_win, context, action, Module, True);
}

void SetGlobalOptions(F_CMD_ARGS)
{
  char *opt;

  /* fvwm_msg(DBG,"SetGlobalOptions","init action == '%s'\n",action); */
  for (action = GetNextSimpleOption(action, &opt); opt;
       action = GetNextSimpleOption(action, &opt))
  {
    /* fvwm_msg(DBG,"SetGlobalOptions"," opt == '%s'\n",opt); */
    /* fvwm_msg(DBG,"SetGlobalOptions"," remaining == '%s'\n",
       action?action:"(NULL)"); */
    if (StrEquals(opt,"WINDOWSHADESHRINKS"))
    {
      Scr.go.WindowShadeScrolls = False;
    }
    else if (StrEquals(opt,"WINDOWSHADESCROLLS"))
    {
      Scr.go.WindowShadeScrolls = True;
    }
    else if (StrEquals(opt,"SMARTPLACEMENTISREALLYSMART"))
    {
      Scr.go.SmartPlacementIsClever = True;
    }
    else if (StrEquals(opt,"SMARTPLACEMENTISNORMAL"))
    {
      Scr.go.SmartPlacementIsClever = False;
    }
    else if (StrEquals(opt,"CLICKTOFOCUSDOESNTPASSCLICK"))
    {
      Scr.go.ClickToFocusPassesClick = False;
    }
    else if (StrEquals(opt,"CLICKTOFOCUSPASSESCLICK"))
    {
      Scr.go.ClickToFocusPassesClick = True;
    }
    else if (StrEquals(opt,"CLICKTOFOCUSDOESNTRAISE"))
    {
      Scr.go.ClickToFocusRaises = False;
    }
    else if (StrEquals(opt,"CLICKTOFOCUSRAISES"))
    {
      Scr.go.ClickToFocusRaises = True;
    }
    else if (StrEquals(opt,"MOUSEFOCUSCLICKDOESNTRAISE"))
    {
      Scr.go.MouseFocusClickRaises = False;
    }
    else if (StrEquals(opt,"MOUSEFOCUSCLICKRAISES"))
    {
      Scr.go.MouseFocusClickRaises = True;
    }
    else if (StrEquals(opt,"NOSTIPLEDTITLES"))
    {
      Scr.go.StipledTitles = False;
    }
    else if (StrEquals(opt,"STIPLEDTITLES"))
    {
      Scr.go.StipledTitles = True;
    }
         /*  RBW - 11/14/1998 - I'll eventually remove these. */
/*
    else if (StrEquals(opt,"STARTSONPAGEMODIFIESUSPOSITION"))
    {
      Scr.go.ModifyUSP = True;
    }
    else if (StrEquals(opt,"STARTSONPAGEHONORSUSPOSITION"))
    {
      Scr.go.ModifyUSP = False;
    }
*/
    else if (StrEquals(opt,"CAPTUREHONORSSTARTSONPAGE"))
    {
      Scr.go.CaptureHonorsStartsOnPage = True;
    }
    else if (StrEquals(opt,"CAPTUREIGNORESSTARTSONPAGE"))
    {
      Scr.go.CaptureHonorsStartsOnPage = False;
    }
    else if (StrEquals(opt,"RECAPTUREHONORSSTARTSONPAGE"))
    {
      Scr.go.RecaptureHonorsStartsOnPage = True;
    }
    else if (StrEquals(opt,"RECAPTUREIGNORESSTARTSONPAGE"))
    {
      Scr.go.RecaptureHonorsStartsOnPage = False;
    }
    else if (StrEquals(opt,"ACTIVEPLACEMENTHONORSSTARTSONPAGE"))
    {
      Scr.go.ActivePlacementHonorsStartsOnPage = True;
    }
    else if (StrEquals(opt,"ACTIVEPLACEMENTIGNORESSTARTSONPAGE"))
    {
      Scr.go.ActivePlacementHonorsStartsOnPage = False;
    }
    else if (StrEquals(opt,"RAISEOVERNATIVEWINDOWS"))
    {
      Scr.go.RaiseHackNeeded = True;
    }
    else if (StrEquals(opt,"IGNORENATIVEWINDOWS"))
    {
      Scr.go.RaiseHackNeeded = False;
    }
    else
      fvwm_msg(ERR,"SetGlobalOptions","Unknown Global Option '%s'",opt);
    /* should never be null, but checking anyways... */
    if (opt)
      free(opt);
  }
  if (opt)
    free(opt);
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
  ApplyDefaultFontAndColors();
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


extern float rgpctMovementDefault[32];
extern int cpctMovementDefault;
extern int cmsDelayDefault;


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
  int n = 0;
  int val = 0;
  int unit = 0;

  n = GetOnePercentArgument(action, &val, &unit);
  if (n != 1 || val < 0)
  {
    Scr.shade_anim_steps = 0;
    return;
  }

  /* we have a 'pixel' suffix if unit != 0; negative values mean pixels */
  Scr.shade_anim_steps = (unit != 0) ? -val : val;

  return;
}
