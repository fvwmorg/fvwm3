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
#include "functions.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

#ifdef WINDOWSHADE
static int shade_anim_steps=0;
#endif

static Boolean ReadMenuFace(char *s, MenuFace *mf, int verbose);
static void FreeMenuFace(Display *dpy, MenuFace *mf);
static void ApplyIconFont(void);
static void ApplyWindowFont(FvwmDecor *fl);
void MaximizeHeight(FvwmWindow *win, int height, int x, int *ret_height,
		    int *ret_y);
void MaximizeWidth(FvwmWindow *win, int *ret_width, int *ret_x, int height,
		   int y);

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
};

/**************************************************************************
 *
 * Moves focus to specified window
 *
 *************************************************************************/
void FocusOn(FvwmWindow *t,Bool FocusByMouse)
{
#ifndef NON_VIRTUAL
  int dx,dy;
  int cx,cy;
#endif
  int x,y;

  if(t == (FvwmWindow *)0)
    return;

  if(t->Desk != Scr.CurrentDesk)
  {
    changeDesks(t->Desk);
  }

#ifndef NON_VIRTUAL
  if(t->flags & ICONIFIED)
  {
    cx = t->icon_xl_loc + t->icon_w_width/2;
    cy = t->icon_y_loc + t->icon_p_height + ICON_HEIGHT/2;
  }
  else
  {
    cx = t->frame_x + t->frame_width/2;
    cy = t->frame_y + t->frame_height/2;
  }

  dx = (cx + Scr.Vx)/Scr.MyDisplayWidth*Scr.MyDisplayWidth;
  dy = (cy +Scr.Vy)/Scr.MyDisplayHeight*Scr.MyDisplayHeight;

  MoveViewport(dx,dy,True);
#endif

  if(t->flags & ICONIFIED)
  {
    x = t->icon_xl_loc + t->icon_w_width/2;
    y = t->icon_y_loc + t->icon_p_height + ICON_HEIGHT/2;
  }
  else
  {
    x = t->frame_x;
    y = t->frame_y;
  }
#if 0 /* don't want to warp the pointer by default anymore */
  if(!(t->flags & ClickToFocus))
    XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, x+2,y+2);
#endif /* 0 */
#if 0 /* don't want to raise anymore either */
  RaiseWindow(t);
#endif /* 0 */

  /* If the window is still not visible, make it visible! */
  if(((t->frame_x + t->frame_height)< 0)||(t->frame_y + t->frame_width < 0)||
     (t->frame_x >Scr.MyDisplayWidth)||(t->frame_y>Scr.MyDisplayHeight))
  {
    SetupFrame(t,0,0,t->frame_width, t->frame_height,False,False);
    if(!(t->flags & ClickToFocus))
      XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, 2,2);
  }
  UngrabEm();
  SetFocus(t->w,t,FocusByMouse);
}



/**************************************************************************
 *
 * Moves pointer to specified window
 *
 *************************************************************************/
void WarpOn(FvwmWindow *t,int warp_x, int x_unit, int warp_y, int y_unit)
{
#ifndef NON_VIRTUAL
  int dx,dy;
  int cx,cy;
#endif
  int x,y;

  if(t == (FvwmWindow *)0 || (t->flags & ICONIFIED && t->icon_w == None))
    return;

  if(t->Desk != Scr.CurrentDesk)
  {
    changeDesks(t->Desk);
  }

#ifndef NON_VIRTUAL
  if(t->flags & ICONIFIED)
  {
    cx = t->icon_xl_loc + t->icon_w_width/2;
    cy = t->icon_y_loc + t->icon_p_height + ICON_HEIGHT/2;
  }
  else
  {
    cx = t->frame_x + t->frame_width/2;
    cy = t->frame_y + t->frame_height/2;
  }

  dx = (cx + Scr.Vx)/Scr.MyDisplayWidth*Scr.MyDisplayWidth;
  dy = (cy +Scr.Vy)/Scr.MyDisplayHeight*Scr.MyDisplayHeight;

  MoveViewport(dx,dy,True);
#endif

  if(t->flags & ICONIFIED)
  {
    x = t->icon_xl_loc + t->icon_w_width/2;
    y = t->icon_y_loc + t->icon_p_height + ICON_HEIGHT/2;
  }
  else
  {
    if (x_unit != Scr.MyDisplayWidth)
      x = t->frame_x + warp_x;
    else
      x = t->frame_x + (t->frame_width - 1) * warp_x / 100;
    if (y_unit != Scr.MyDisplayHeight)
      y = t->frame_y + warp_y;
    else
      y = t->frame_y + (t->frame_height - 1) * warp_y / 100;
  }
  if (warp_x >= 0 && warp_y >= 0) {
    XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, x, y);
  }
  RaiseWindow(t);

  /* If the window is still not visible, make it visible! */
  if(((t->frame_x + t->frame_height)< 0)||(t->frame_y + t->frame_width < 0)||
     (t->frame_x >Scr.MyDisplayWidth)||(t->frame_y>Scr.MyDisplayHeight))
  {
    SetupFrame(t,0,0,t->frame_width, t->frame_height,False,False);
    XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, 2,2);
  }
  UngrabEm();
}


static int 
truncate_to_multiple (int x, int m)
{
  return (x < 0) ? (m * (x / m - 1)) : (m * (x / m)); 
}

/***********************************************************************
 *
 *  Procedure:
 *	(Un)Maximize a window.
 *
 ***********************************************************************/
void Maximize(F_CMD_ARGS)
{
  int new_width, new_height,new_x,new_y;
  int page_x, page_y;
  int val1, val2, val1_unit, val2_unit;
  int toggle;
  char *token;
  char *taction;
  Bool grow_x = False;
  Bool grow_y = False;

  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
    return;

  if(tmp_win == NULL)
    return;

  if(check_allowed_function2(F_MAXIMIZE,tmp_win) == 0)
  {
    XBell(dpy, 0);
    return;
  }
  toggle = ParseToggleArgument(action, &action, -1, 0);
  if (((toggle == 1) && (tmp_win->flags & MAXIMIZED)) ||
      ((toggle == 0) && !(tmp_win->flags & MAXIMIZED)))
    return;

  /* parse first parameter */
  val1_unit = Scr.MyDisplayWidth;
  taction = PeekToken(action, &token);
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
  PeekToken(taction, &token);
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

  if (tmp_win->flags & MAXIMIZED)
  {
    tmp_win->flags &= ~MAXIMIZED;
    /* Unmaximizing is slightly tricky since we want the window to
       stay on the same page, even if we have move to a different page
       in the meantime. the orig values are absolute! */
    if (tmp_win->flags & STICKY) 
      {
	/* make sure we keep it on screen while unmaximizing */ 
	new_x=tmp_win->orig_x-truncate_to_multiple(tmp_win->orig_x,Scr.MyDisplayWidth);
	new_y=tmp_win->orig_y-truncate_to_multiple(tmp_win->orig_y,Scr.MyDisplayHeight); 
      }
    else 
      {
	new_x = tmp_win->orig_x - Scr.Vx;
	new_y = tmp_win->orig_y - Scr.Vy;
      }
    new_width = tmp_win->orig_wd;
#ifdef WINDOWSHADE
    if (tmp_win->buttons & WSHADE)
      new_height = tmp_win->title_height + tmp_win->boundary_width;
    else
#endif
      new_height = tmp_win->orig_ht;
    SetupFrame(tmp_win, new_x, new_y, new_width, new_height, TRUE, False);
    SetBorder(tmp_win, True, True, True, None);
  }
  else
  {
#ifdef WINDOWSHADE
    if (tmp_win->buttons & WSHADE)
      new_height = tmp_win->orig_ht;
    else
#endif
      new_height = tmp_win->frame_height;
    new_width = tmp_win->frame_width;

    page_x = truncate_to_multiple (tmp_win->frame_x,Scr.MyDisplayWidth);
    page_y = truncate_to_multiple (tmp_win->frame_y,Scr.MyDisplayHeight);

    new_x = tmp_win->frame_x;
    new_y = tmp_win->frame_y;

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
    tmp_win->flags |= MAXIMIZED;
    ConstrainSize (tmp_win, &new_width, &new_height, False, 0, 0);
    tmp_win->maximized_ht = new_height;
#ifdef WINDOWSHADE
    if (tmp_win->buttons & WSHADE)
      new_height = tmp_win->frame_height;
#endif
    SetupFrame(tmp_win,new_x,new_y,new_width,new_height,TRUE,False);
    /*   SetBorder(tmp_win,Scr.Hilite == tmp_win,True,True,None);*/
  }
}

void MaximizeHeight(FvwmWindow *win, int win_width, int win_x, int *win_height,
		    int *win_y)
{
  FvwmWindow *cwin;
  int x11, x12, x21, x22;
  int y11, y12, y21, y22;
  int new_y1, new_y2;

  x11 = win_x;             /* Start x */
  y11 = *win_y;            /* Start y */
  x12 = x11 + win_width;   /* End x   */
  y12 = y11 + *win_height; /* End y   */
  new_y1 = 0;
  new_y2 = Scr.MyDisplayHeight;

  for (cwin = Scr.FvwmRoot.next; cwin; cwin = cwin->next) {
    if ((cwin == win) ||
	((cwin->Desk != win->Desk) && (!(cwin->flags & STICKY)))) {
      continue;
    }
    if (cwin->flags & ICONIFIED) {
      if(cwin->icon_w == None || cwin->flags & ICON_UNMAPPED)
	continue;
      x21 = cwin->icon_x_loc;
      y21 = cwin->icon_y_loc;
      x22 = x21 + cwin->icon_p_width;
      y22 = y21 + cwin->icon_p_height + cwin->icon_w_height;
    } else {
      x21 = cwin->frame_x;
      y21 = cwin->frame_y;
      x22 = x21 + cwin->frame_width;
      y22 = y21 + cwin->frame_height;
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

void MaximizeWidth(FvwmWindow *win, int *win_width, int *win_x, int win_height,
		   int win_y)
{
  FvwmWindow *cwin;
  int x11, x12, x21, x22;
  int y11, y12, y21, y22;
  int new_x1, new_x2;

  x11 = *win_x;            /* Start x */
  y11 = win_y;             /* Start y */
  x12 = x11 + *win_width;  /* End x   */
  y12 = y11 + win_height;  /* End y   */
  new_x1 = 0;
  new_x2 = Scr.MyDisplayWidth;

  for (cwin = Scr.FvwmRoot.next; cwin; cwin = cwin->next) {
    if ((cwin == win) ||
	((cwin->Desk != win->Desk) && (!(cwin->flags & STICKY)))) {
      continue;
    }
    if (cwin->flags & ICONIFIED) {
      if(cwin->icon_w == None || cwin->flags & ICON_UNMAPPED)
	continue;
      x21 = cwin->icon_x_loc;
      y21 = cwin->icon_y_loc;
      x22 = x21 + cwin->icon_p_width;
      y22 = y21 + cwin->icon_p_height + cwin->icon_w_height;
    } else {
      x21 = cwin->frame_x;
      y21 = cwin->frame_y;
      x22 = x21 + cwin->frame_width;
      y22 = y21 + cwin->frame_height;
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

#ifdef WINDOWSHADE
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

  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
    return;
  if (tmp_win == NULL)
    return;

  if (!(tmp_win->flags & TITLE)) {
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
    toggle = (tmp_win->buttons & WSHADE) ? 0 : 1;

  /* calcuate the step size */
  if (shade_anim_steps > 0)
    step = tmp_win->orig_ht/shade_anim_steps;
  else if (shade_anim_steps < 0) /* if it's -ve it means pixels */
    step = -shade_anim_steps;
  if (step <= 0)  /* We don't want an endless loop, do we? */
    step = 1;

  if ((tmp_win->buttons & WSHADE) && toggle == 0)
  {
    /* unshade window */
    tmp_win->buttons &= ~WSHADE;
    new_x = tmp_win->frame_x;
    new_y = tmp_win->frame_y;
    if (tmp_win->flags & MAXIMIZED)
      new_height = tmp_win->maximized_ht;
    else
      new_height = tmp_win->orig_ht;

    /* this is necessary if the maximized state has changed while shaded */
    SetupFrame(tmp_win, new_x, new_y, tmp_win->frame_width, new_height,
               True, True);

    if (shade_anim_steps != 0) {
      h = tmp_win->title_height+tmp_win->boundary_width;
      if (Scr.go.WindowShadeScrolls)
        XMoveWindow(dpy, tmp_win->w, 0, - (new_height-h));
      y = h - new_height;
      old_h = tmp_win->frame_height;
      while (h < new_height) {
        XResizeWindow(dpy, tmp_win->frame, tmp_win->frame_width, h);
        XResizeWindow(dpy, tmp_win->Parent,
                      tmp_win->frame_width - 2 * tmp_win->boundary_width,
                      max(h - 2 * tmp_win->boundary_width 
                          - tmp_win->title_height, 1));
        if (Scr.go.WindowShadeScrolls)
          XMoveWindow(dpy, tmp_win->w, 0, y);
        tmp_win->frame_height = h;
        /* way too flickery
        SetBorder(tmp_win, tmp_win == Scr.Hilite, True, True, tmp_win->frame);
        */
        BroadcastConfig(M_CONFIGURE_WINDOW, tmp_win);
        FlushOutputQueues();
        XSync(dpy, 0);
        h+=step;
        y+=step;
      }
      tmp_win->frame_height = old_h;
      XMoveWindow(dpy, tmp_win->w, 0, 0);
    }
    SetupFrame(tmp_win, new_x, new_y, tmp_win->frame_width, new_height,
               True, False);
    BroadcastPacket(M_DEWINDOWSHADE, 3, tmp_win->w, tmp_win->frame,
                    (unsigned long)tmp_win);
  }
  else if (!(tmp_win->buttons & WSHADE) && toggle == 1)
  {
  /* shade window */
  tmp_win->buttons |= WSHADE;

  if (shade_anim_steps != 0) {
    XLowerWindow(dpy, tmp_win->w);
    h = tmp_win->frame_height;
    y = 0;
    old_h = tmp_win->frame_height;
    while (h > tmp_win->title_height+tmp_win->boundary_width) {
      if (Scr.go.WindowShadeScrolls)
        XMoveWindow(dpy, tmp_win->w, 0, y);
        XResizeWindow(dpy, tmp_win->frame, tmp_win->frame_width, h);
        XResizeWindow(dpy, tmp_win->Parent,
                      tmp_win->frame_width - 2 * tmp_win->boundary_width,
                      max(h - 2 * tmp_win->boundary_width 
                          - tmp_win->title_height, 1));
        tmp_win->frame_height = h;
        /* way too flickery
        SetBorder(tmp_win, tmp_win == Scr.Hilite, True, True, tmp_win->frame);
        */
        BroadcastConfig(M_CONFIGURE_WINDOW, tmp_win);
        FlushOutputQueues();
        XSync(dpy, 0);
        h-=step;
        y-=step;
      }
      tmp_win->frame_height = old_h;
      if (Scr.go.WindowShadeScrolls)
        XMoveWindow(dpy, tmp_win->w, 0, 0);
    }
    SetupFrame(tmp_win, tmp_win->frame_x, tmp_win->frame_y,
               tmp_win->frame_width, tmp_win->title_height
               + tmp_win->boundary_width, False, False);
    BroadcastPacket(M_WINDOWSHADE, 3, tmp_win->w, tmp_win->frame,
                    (unsigned long)tmp_win);
  }
  FlushOutputQueues();
  XSync(dpy, 0);

}
#endif /* WINDOWSHADE */


void Bell(F_CMD_ARGS)
{
  XBell(dpy, 0);
}


#ifdef USEDECOR
static FvwmDecor *last_decor = NULL, *cur_decor = NULL;
#endif
static MenuRoot *last_menu=NULL;
static FvwmFunction *last_func=NULL;
void add_item_to_menu(F_CMD_ARGS)
{
  MenuRoot *mr;
  MenuRoot *mrPrior;
  char *token, *rest,*item;

  rest = GetNextToken(action,&token);
  if (!token)
    return;
  mr = FollowMenuContinuations(FindPopup(token),&mrPrior);
  if(mr == NULL)
    mr = NewMenuRoot(token);

  /* Set + state to last menu */
  last_menu = mr;
#ifdef USEDECOR
  last_decor = NULL;
#endif
  last_func = NULL;

  rest = GetNextToken(rest,&item);
  AddToMenu(mr, item,rest,TRUE /* pixmap scan */, TRUE);
  if (item)
    free(item);
  /* These lines are correct! We must not release token if the string is
     empty.  WHY??
   * It cannot be NULL! GetNextToken never returns an empty string! */
  if (*token)
    free(token);

  MakeMenu(mr);
  return;
}


void add_another_item(F_CMD_ARGS)
{
#ifdef USEDECOR
  extern void AddToDecor(FvwmDecor *, char *);
#endif
  MenuRoot *mr;
  MenuRoot *mrPrior;
  char *rest,*item;

  if (last_menu) {
    mr = FollowMenuContinuations(last_menu,&mrPrior);
    if(mr == NULL)
      return;
    rest = GetNextToken(action,&item);
    AddToMenu(mr, item,rest,TRUE /* pixmap scan */, FALSE);
    if (item)
      free(item);
    MakeMenu(mr);
  }
  else if (last_func) {
    AddToFunction(last_func, action);
  }
#ifdef USEDECOR
  else if (last_decor) {
    FvwmDecor *tmp = &Scr.DefaultDecor;
    for (; tmp; tmp = tmp->next)
      if (tmp == last_decor)
	break;
    if (!tmp)
      return;
    AddToDecor(tmp, action);
  }
#endif /* USEDECOR */
}

void destroy_menu(F_CMD_ARGS)
{
  MenuRoot *mr;
  MenuRoot *mrContinuation;

  char *token, *rest;

  rest = GetNextToken(action,&token);
  if (!token)
    return;
  mr = FindPopup(token);
  last_menu = NULL;
  free(token);
  while (mr)
  {
    mrContinuation = mr->continuation; /* save continuation before destroy */
    DestroyMenu(mr);
    mr = mrContinuation;
  }
  return;
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
  last_func = NULL;
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
  last_func = func;
#ifdef USEDECOR
  last_decor = NULL;
#endif
  last_menu = NULL;

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
#ifndef NON_VIRTUAL
  int virtual_x, virtual_y;
  int pan_x, pan_y;
  int x_pages, y_pages;
#endif

  if (GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit) != 2)
  {
    fvwm_msg(ERR, "movecursor", "CursorMove needs 2 arguments");
    return;
  }

  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
                 &x, &y, &JunkX, &JunkY, &JunkMask);

  x = x + val1 * val1_unit / 100;
  y = y + val2 * val2_unit / 100;

#ifndef NON_VIRTUAL
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
#endif

  XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
	       Scr.MyDisplayHeight, x, y);
  return;
}


void PlaceAgain_func(F_CMD_ARGS)
{
  int x,y;
  char *token;

  if (DeferExecution(eventp, &w, &tmp_win, &context, SELECT, ButtonRelease))
    return;

  /* Find new position for window */
  SmartPlacement(tmp_win,tmp_win->frame_width,tmp_win->frame_height, &x, &y,
		 0,0);

  /* Possibly animate the movement */
  GetNextToken(action, &token);
  if(token && StrEquals("ANIM", token))
    AnimatedMoveOfWindow(tmp_win->frame, -1, -1, x, y, FALSE, -1, NULL);

  SetupFrame(tmp_win,x,y,tmp_win->frame_width, tmp_win->frame_height, FALSE,
	     False);

  return;
}


void iconify_function(F_CMD_ARGS)
{
  int toggle;

  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT, ButtonRelease))
    return;

  toggle = ParseToggleArgument(action, NULL, -1, 0);
  if (toggle == -1)
  {
    if (GetIntegerArguments(action, NULL, &toggle, 1) > 0)
      {
	if (toggle > 0)
	  toggle = 1;
	else if (toggle < 0)
	  toggle = 0;
	else
	  toggle = -1;
      }
  }
  if (toggle == -1)
    toggle = (tmp_win->flags & ICONIFIED) ? 0 : 1;

  if (tmp_win->flags & ICONIFIED)
  {
    if (toggle == 0)
      DeIconify(tmp_win);
  }
  else
  {
    if (toggle == 1)
    {
      if(check_allowed_function2(F_ICONIFY,tmp_win) == 0)
      {
	XBell(dpy, 0);
	return;
      }
      Iconify(tmp_win,eventp->xbutton.x_root-5,eventp->xbutton.y_root-5);
    }
  }
}

void raise_function(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
    return;

  RaiseWindow(tmp_win);
}

void lower_function(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT, ButtonRelease))
    return;

  LowerWindow(tmp_win);
}

void destroy_function(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, DESTROY, ButtonRelease))
    return;

  if(check_allowed_function2(F_DESTROY,tmp_win) == 0)
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
  if (DeferExecution(eventp,&w,&tmp_win,&context, DESTROY,ButtonRelease))
    return;

  if(check_allowed_function2(F_DELETE,tmp_win) == 0)
  {
    XBell(dpy, 0);
    return;
  }

  if (tmp_win->flags & DoesWmDeleteWindow)
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
  if (DeferExecution(eventp,&w,&tmp_win,&context, DESTROY,ButtonRelease))
    return;

  if(check_allowed_function2(F_CLOSE,tmp_win) == 0)
  {
    XBell(dpy, 0);
    return;
  }

  if (tmp_win->flags & DoesWmDeleteWindow)
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

  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
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

  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;

  toggle = ParseToggleArgument(action, &action, -1, 0);
  if ((toggle == 1 && (tmp_win->flags & STICKY)) ||
      (toggle == 0 && !(tmp_win->flags & STICKY)))
    return;

  if(tmp_win->flags & STICKY)
  {
    tmp_win->flags &= ~STICKY;
  }
  else
  {
    tmp_win->flags |= STICKY;
    move_window_doit(eventp, w, tmp_win, context, "", Module, FALSE, TRUE);
    /* move_window_doit resets the STICKY flag, so we must set it after the
     * call! */
    tmp_win->flags |= STICKY;
  }
  BroadcastConfig(M_CONFIGURE_WINDOW,tmp_win);
  SetTitleBar(tmp_win,(Scr.Hilite==tmp_win),True);
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
      DispatchEvent ();
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

void flip_focus_func(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;

  /* Reorder the window list */
  FocusOn(tmp_win,TRUE);
}


void focus_func(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;

  FocusOn(tmp_win,FALSE);
}


void warp_func(F_CMD_ARGS)
{
   int val1_unit, val2_unit, n;
   int val1, val2;

  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;

   n = GetTwoArguments (action, &val1, &val2, &val1_unit, &val2_unit);

   if (n == 2)
     WarpOn (tmp_win, val1, val1_unit, val2, val2_unit);
   else
     WarpOn (tmp_win, 0, 0, 0, 0);
}


static void menu_func(F_CMD_ARGS, Bool fStaysUp)
{
  extern int menuFromFrameOrWindowOrTitlebar;
  MenuRoot *menu;
  MenuItem *miExecuteAction = NULL;
  MenuOptions mops;
  char *menu_name = NULL;
  XEvent *teventp;

  mops.flags.allflags = 0;
  action = GetNextToken(action,&menu_name);
  action = GetMenuOptions(action,w,tmp_win,NULL,&mops);
  while (action && *action && isspace(*action))
    action++;
  if (action && *action == 0)
    action = NULL;
  menu = FindPopup(menu_name);
  if(menu == NULL)
  {
    if(menu_name)
    {
      fvwm_msg(ERR,"menu_func","No such menu %s",menu_name);
      free(menu_name);
    }
    return;
  }
  if(menu_name)
    free(menu_name);
  menuFromFrameOrWindowOrTitlebar = FALSE;

  if (!action && eventp && eventp->type == KeyPress)
    teventp = (XEvent *)1;
  else
    teventp = eventp;
  if ((do_menu(menu, NULL, &miExecuteAction, 0, fStaysUp, teventp, &mops) ==
       MENU_DOUBLE_CLICKED) && action)
  {
    ExecuteFunction(action,tmp_win,eventp,context,*Module);
  }
}

/* the function for the "Popup" command */
void popup_func(F_CMD_ARGS)
{
  menu_func(eventp, w, tmp_win, context, action, Module, False);
}

/* the function for the "Menu" command */
void staysup_func(F_CMD_ARGS)
{
  menu_func(eventp, w, tmp_win, context, action, Module, True);
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

void raiselower_func(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
    return;

  if((tmp_win == Scr.LastWindowRaised)||
     (tmp_win->flags & VISIBLE))
  {
    LowerWindow(tmp_win);
  }
  else
  {
    RaiseWindow(tmp_win);
  }
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
    fvwm_msg(ERR,"SetSnapAttraction",
	     "SnapAttraction requires at least 1 argument");
    return;
  }
  Scr.SnapAttraction = val;

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
    Scr.SnapMode = 0;
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
    fvwm_msg(ERR,"SetSnapGrid","SetSnapGrid requires 2 arguments");
    return;
  }

  Scr.SnapGridX = val[0];
  if(Scr.SnapGridX < 1)
    { Scr.SnapGridX = 1;}
  Scr.SnapGridY = val[1];
  if(Scr.SnapGridY < 1)
    { Scr.SnapGridY = 1;}
}


void SetXOR(F_CMD_ARGS)
{
  int val;
  XGCValues gcv;
  unsigned long gcm;

  if(GetIntegerArguments(action, NULL, &val, 1) != 1)
  {
    fvwm_msg(ERR,"SetXOR","XORValue requires 1 argument");
    return;
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
    fvwm_msg(ERR,"SetXORPixmap","XORPixmap requires 1 argument");
    return;
  }

  /* search for pixmap */
  GCPicture = CachePicture(dpy, Scr.Root, NULL, PixmapName, Scr.ColorLimit);
  free(PixmapName);
  if (GCPicture == NULL) {
    fvwm_msg(ERR,"SetXORPixmap","Can't find pixmap %s", PixmapName);
    return;
  }

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
    fvwm_msg(ERR,"SetOpaque","OpaqueMoveSize requires 1 argument");
    return;
  }

  Scr.OpaqueSize = val;
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
  if(Scr.d_depth > 2)
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


static void SafeDefineCursor(Window w, Cursor cursor)
{
  if (w) XDefineCursor(dpy,w,cursor);
}

void CursorStyle(F_CMD_ARGS)
{
  char *cname=NULL, *newcursor=NULL;
  char *errpos = NULL, *path = NULL;
  char *fore = NULL, *back = NULL;
  XColor colors[2];
  int index,nc,i;
  FvwmWindow *fw;
  MenuRoot *mr;

  action = GetNextToken(action,&cname);
  action = GetNextToken(action,&newcursor);

  if (!cname || !newcursor)
  {
    fvwm_msg(ERR,"CursorStyle","Bad cursor style");
    if (cname)
      free(cname);
    if (newcursor)
      free(newcursor);
    return;
  }
  if (StrEquals("POSITION",cname)) index = POSITION;
  else if (StrEquals("DEFAULT",cname)) index = DEFAULT;
  else if (StrEquals("SYS",cname)) index = SYS;
  else if (StrEquals("TITLE",cname)) index = TITLE_CURSOR;
  else if (StrEquals("MOVE",cname)) index = MOVE;
  else if (StrEquals("MENU",cname)) index = MENU;
  else if (StrEquals("WAIT",cname)) index = WAIT;
  else if (StrEquals("SELECT",cname)) index = SELECT;
  else if (StrEquals("DESTROY",cname)) index = DESTROY;
  else if (StrEquals("LEFT",cname)) index = LEFT;
  else if (StrEquals("RIGHT",cname)) index = RIGHT;
  else if (StrEquals("TOP",cname)) index = TOP;
  else if (StrEquals("BOTTOM",cname)) index = BOTTOM;
  else if (StrEquals("TOP_LEFT",cname)) index = TOP_LEFT;
  else if (StrEquals("TOP_RIGHT",cname)) index = TOP_RIGHT;
  else if (StrEquals("BOTTOM_LEFT",cname)) index = BOTTOM_LEFT;
  else if (StrEquals("BOTTOM_RIGHT",cname)) index = BOTTOM_RIGHT;
  else
  {
    fvwm_msg(ERR,"CursorStyle","Unknown cursor name %s",cname);
    free(cname);
    free(newcursor);
    return;
  }
  free(cname);

  nc = strtol (newcursor, &errpos, 10);
  if (errpos && *errpos == '\0')
    {
      /* newcursor was a number */
      if ((nc < 0) || (nc >= XC_num_glyphs) || ((nc % 2) != 0))
	{
	  fvwm_msg(ERR, "CursorStyle", "Bad cursor number %s", newcursor);
	  free(newcursor);
	  return;
	}
      free(newcursor);

      /* replace the cursor defn */
      if (Scr.FvwmCursors[index]) XFreeCursor(dpy,Scr.FvwmCursors[index]);
      Scr.FvwmCursors[index] = XCreateFontCursor(dpy,nc);
    }
  else
    {
      /* newcursor was not a number */
#ifdef XPM
      XpmAttributes xpm_attributes;
      Pixmap source, mask;

      path = findImageFile (newcursor, NULL, R_OK);
      if (!path)
	{
	  fvwm_msg (ERR, "CursorStyle", "Cursor xpm not found %s", newcursor);
	  free (newcursor);
	  return;
	}

      xpm_attributes.depth = 1; /* we need source to be a bitmap */
      xpm_attributes.valuemask = XpmSize | XpmDepth | XpmHotspot;
      if (XpmReadFileToPixmap (dpy, Scr.Root, path, &source, &mask,
			       &xpm_attributes) != XpmSuccess)
	{
	  fvwm_msg (ERR, "CursorStyle", "Error reading cursor xpm %s",
		    newcursor);
	  free (newcursor);
	  return;
	}

      colors[0].pixel = BlackPixel(dpy, Scr.screen);
      colors[1].pixel = WhitePixel(dpy, Scr.screen);
      XQueryColors (dpy, PictureCMap, colors, 2);

      if (Scr.FvwmCursors[index]) XFreeCursor (dpy, Scr.FvwmCursors[index]);
      Scr.FvwmCursors[index] =
	XCreatePixmapCursor (dpy, source, mask,
			     &(colors[0]), &(colors[1]),
			     xpm_attributes.x_hotspot,
			     xpm_attributes.y_hotspot);

      free (newcursor);
      free (path);
#else /* ! XPM */
      fvwm_msg (ERR, "CursorStyle", "Bad cursor number %s", newcursor);
      free (newcursor);
      return;
#endif
    }

   /* look for optional color arguments */
  action = GetNextToken(action, &fore);
  action = GetNextToken(action, &back);
  if (fore && back)
    {
      colors[0].pixel = GetColor (fore);
      colors[1].pixel = GetColor (back);
      XQueryColors (dpy, PictureCMap, colors, 2);
      XRecolorCursor (dpy, Scr.FvwmCursors[index], &(colors[0]), &(colors[1]));
    }

  /* redefine all the windows using cursors */
  fw = Scr.FvwmRoot.next;
  while(fw)
  {
    for (i=0;i<4;i++)
    {
      SafeDefineCursor(fw->corners[i],Scr.FvwmCursors[TOP_LEFT+i]);
      SafeDefineCursor(fw->sides[i],Scr.FvwmCursors[TOP+i]);
    }
    for (i=0;i<Scr.nr_left_buttons;i++)
    {
      SafeDefineCursor(fw->left_w[i],Scr.FvwmCursors[SYS]);
    }
    for (i=0;i<Scr.nr_right_buttons;i++)
    {
      SafeDefineCursor(fw->right_w[i],Scr.FvwmCursors[SYS]);
    }
    SafeDefineCursor(fw->title_w, Scr.FvwmCursors[TITLE_CURSOR]);
    fw = fw->next;
  }

  /* Do the menus for good measure */
  mr = Scr.menus.all;
  while(mr)
  {
    SafeDefineCursor(mr->w,Scr.FvwmCursors[MENU]);
    mr = mr->next;
  }
}

static MenuStyle *FindMenuStyle(char *name)
{
  MenuStyle *ms = Scr.menus.DefaultStyle;

  while(ms)
  {
     if(strcasecmp(ms->name,name)==0)
       return ms;
     ms = ms->next;
  }
  return NULL;
}

static void FreeMenuStyle(MenuStyle *ms)
{
  MenuRoot *mr;
  MenuStyle *before = Scr.menus.DefaultStyle;

  if (!ms)
    return;
  mr = Scr.menus.all;
  while(mr)
  {
    if(mr->ms == ms)
      mr->ms = Scr.menus.DefaultStyle;
    mr = mr->next;
  }
  if(ms->look.MenuGC)
    XFreeGC(dpy, ms->look.MenuGC);
  if(ms->look.MenuActiveGC)
    XFreeGC(dpy, ms->look.MenuActiveGC);
  if(ms->look.MenuActiveBackGC)
    XFreeGC(dpy, ms->look.MenuActiveBackGC);
  if(ms->look.MenuReliefGC)
    XFreeGC(dpy, ms->look.MenuReliefGC);
  if(ms->look.MenuStippleGC)
    XFreeGC(dpy, ms->look.MenuStippleGC);
  if(ms->look.MenuShadowGC)
    XFreeGC(dpy, ms->look.MenuShadowGC);
  if (ms->look.sidePic)
    DestroyPicture(dpy, ms->look.sidePic);
  if (ms->look.f.hasSideColor == 1)
    FreeColors(&ms->look.sideColor, 1);

  while(before->next != ms)
    /* Not too many checks, may segfaults in race conditions */
    before = before->next;

  before->next = ms->next;
  free(ms->name);
  free(ms);
}

void DestroyMenuStyle(F_CMD_ARGS)
{
  MenuStyle *ms = NULL;
  char *name = NULL;
  MenuRoot *mr;

  action = GetNextToken(action,&name);
  if (name == NULL)
  {
    fvwm_msg(ERR,"DestroyMenuStyle", "needs one parameter");
    return;
  }

  ms = FindMenuStyle(name);
  if(ms == NULL)
    fvwm_msg(ERR,"DestroyMenuStyle", "cannot find style %s", name);
  else if (ms == Scr.menus.DefaultStyle)
    fvwm_msg(ERR,"DestroyMenuStyle", "cannot destroy Default Menu Face");
  else
  {
    FreeMenuFace(dpy, &ms->look.face);
    FreeMenuStyle(ms);
    MakeMenus();
  }
  free(name);
  for (mr = Scr.menus.all; mr; mr = mr->next)
  {
    if (mr->ms == ms)
      mr->ms = Scr.menus.DefaultStyle;
  }
  MakeMenus();
}

static void UpdateMenuStyle(MenuStyle *ms)
{
  XGCValues gcv;
  unsigned long gcm;

  if (ms->look.pStdFont && ms->look.pStdFont != &Scr.StdFont)
    {
      ms->look.pStdFont->y = ms->look.pStdFont->font->ascent;
      ms->look.pStdFont->height =
	ms->look.pStdFont->font->ascent +
	ms->look.pStdFont->font->descent;
    }
  ms->look.EntryHeight =
    ms->look.pStdFont->height + HEIGHT_EXTRA;

  /* calculate colors based on foreground */
  if (!ms->look.f.hasActiveFore)
    ms->look.MenuActiveColors.fore=ms->look.MenuColors.fore;

  /* calculate colors based on background */
  if (!ms->look.f.hasActiveBack)
    ms->look.MenuActiveColors.back = ms->look.MenuColors.back;
  if (!ms->look.f.hasStippleFore)
    ms->look.MenuStippleColors.fore = ms->look.MenuColors.back;
  if(Scr.d_depth > 2) {                 /* if not black and white */
    ms->look.MenuRelief.back = GetShadow(ms->look.MenuColors.back);
    ms->look.MenuRelief.fore = GetHilite(ms->look.MenuColors.back);
  } else {                              /* black and white */
    ms->look.MenuRelief.back = GetColor("black");
    ms->look.MenuRelief.fore = GetColor("white");
  }
  ms->look.MenuStippleColors.back = ms->look.MenuColors.back;

  /* make GC's */
  gcm = GCFunction|GCFont|GCLineWidth|GCForeground|GCBackground;
  gcv.font = ms->look.pStdFont->font->fid;
  gcv.function = GXcopy;
  gcv.line_width = 0;

  gcv.foreground = ms->look.MenuRelief.fore;
  gcv.background = ms->look.MenuRelief.back;
  if(ms->look.MenuReliefGC)
    XChangeGC(dpy, ms->look.MenuReliefGC, gcm, &gcv);
  else
    ms->look.MenuReliefGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  gcv.foreground = ms->look.MenuRelief.back;
  gcv.background = ms->look.MenuRelief.fore;
  if(ms->look.MenuShadowGC)
    XChangeGC(dpy, ms->look.MenuShadowGC, gcm, &gcv);
  else
    ms->look.MenuShadowGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  gcv.foreground = (ms->look.f.hasActiveBack) ?
    ms->look.MenuActiveColors.back : ms->look.MenuRelief.back;
  gcv.background = (ms->look.f.hasActiveFore) ?
    ms->look.MenuActiveColors.fore : ms->look.MenuRelief.fore;
  if(ms->look.MenuActiveBackGC)
    XChangeGC(dpy, ms->look.MenuActiveBackGC, gcm, &gcv);
  else
    ms->look.MenuActiveBackGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  gcv.foreground = ms->look.MenuColors.fore;
  gcv.background = ms->look.MenuColors.back;
  if(ms->look.MenuGC)
    XChangeGC(dpy, ms->look.MenuGC, gcm, &gcv);
  else
    ms->look.MenuGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  gcv.foreground = ms->look.MenuActiveColors.fore;
  gcv.background = ms->look.MenuActiveColors.back;
  if(ms->look.MenuActiveGC)
    XChangeGC(dpy, ms->look.MenuActiveGC, gcm, &gcv);
  else
    ms->look.MenuActiveGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  if(Scr.d_depth < 2)
  {
    gcv.fill_style = FillStippled;
    gcv.stipple = Scr.gray_bitmap;
    gcm|=GCStipple|GCFillStyle; /* no need to reset fg/bg, FillStipple wins */
  }
  else
  {
    gcv.foreground = ms->look.MenuStippleColors.fore;
    gcv.background = ms->look.MenuStippleColors.back;
  }
  if(ms->look.MenuStippleGC)
    XChangeGC(dpy, ms->look.MenuStippleGC, gcm, &gcv);
  else
    ms->look.MenuStippleGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
}


static int GetMenuStyleIndex(char *option)
{
  char *optlist[] = {
    "fvwm", "mwm", "win",
    "Foreground", "Background", "Greyed",
    "HilightBack", "HilightBackOff",
    "ActiveFore", "ActiveForeOff",
    "Hilight3DThick", "Hilight3DThin", "Hilight3DOff",
    "Animation", "AnimationOff",
    "Font",
    "MenuFace",
    "PopupDelay", "PopupOffset",
    "TitleWarp", "TitleWarpOff",
    "TitleUnderlines0", "TitleUnderlines1", "TitleUnderlines2",
    "SeparatorsLong", "SeparatorsShort",
    "TrianglesSolid", "TrianglesRelief",
    "PopupImmediately", "PopupDelayed",
    "DoubleClickTime",
    "SidePic", "SideColor",
    "PopupAsRootmenu", "PopupAsSubmenu",
    NULL
  };
  return GetTokenIndex(option, optlist, 0, NULL);
}

static void NewMenuStyle(F_CMD_ARGS)
{
  char *name;
  char *option = NULL;
  char *optstring = NULL;
  char *nextarg;
  char *args = NULL;
  char *arg1;
  MenuStyle *ms;
  MenuStyle *tmpms;
  Bool is_initialised = True;
  Bool gc_changed = False;
  Bool is_default_style = False;
  int val[2];
  int n;
  XFontStruct *xfs = NULL;
  int i;

  action = GetNextToken(action, &name);
  if (!name)
    {
      fvwm_msg(ERR,"NewMenuStyle", "error in %s style specification",action);
      return;
    }

  tmpms = (MenuStyle *)safemalloc(sizeof(MenuStyle));
  memset(tmpms, 0, sizeof(MenuStyle));
  ms = FindMenuStyle(name);
  if (ms)
    {
      /* copy the structure over our temporary menu face. */
      memcpy(tmpms, ms, sizeof(MenuStyle));
      if (ms == Scr.menus.DefaultStyle)
	is_default_style = True;
      free(name);
    }
  else
    {
      tmpms->name = name;
      is_initialised = False;
    }

  /* Parse the options. */
  while (action && *action)
    {
      if (is_initialised == False)
	{
	  /* some default configuration goes here for the new menu style */
	  tmpms->look.MenuColors.back = GetColor("white");
	  tmpms->look.MenuColors.fore = GetColor("black");
	  tmpms->look.pStdFont = &Scr.StdFont;
	  tmpms->look.face.type = SimpleMenu;
	  tmpms->look.f.hasActiveFore = 0;
	  tmpms->look.f.hasActiveBack = 0;
	  tmpms->feel.f.PopupAsRootmenu = 0;
	  gc_changed = True;
	  option = "fvwm";
	}
      else
	{
	  /* Read next option specification (delimited by a comma or \0). */
	  args = action;
	  action = GetQuotedString(action, &optstring, ",", NULL, NULL, NULL);
	  if (!optstring)
	    break;

	  args = GetNextToken(optstring, &option);
	  if (!option)
	    {
	      free(optstring);
	      break;
	    }
	  nextarg = GetNextToken(args, &arg1);
	}

      switch((i = GetMenuStyleIndex(option)))
      {
      case 0: /* fvwm */
      case 1: /* mwm */
      case 2: /* win */
	if (i == 0) {
	  tmpms->feel.PopupOffsetPercent = 67;
	  tmpms->feel.PopupOffsetAdd = 0;
	  tmpms->feel.f.PopupImmediately = 0;
	  tmpms->feel.f.TitleWarp = 1;
	  tmpms->look.ReliefThickness = 1;
	  tmpms->look.TitleUnderlines = 1;
	  tmpms->look.f.LongSeparators = 0;
	  tmpms->look.f.TriangleRelief = 1;
	  tmpms->look.f.Hilight = 0;
	} else if (i == 1) {
	  tmpms->feel.PopupOffsetPercent = 100;
	  tmpms->feel.PopupOffsetAdd = -3;
	  tmpms->feel.f.PopupImmediately = 1;
	  tmpms->feel.f.TitleWarp = 0;
	  tmpms->look.ReliefThickness = 2;
	  tmpms->look.TitleUnderlines = 2;
	  tmpms->look.f.LongSeparators = 1;
	  tmpms->look.f.TriangleRelief = 1;
	  tmpms->look.f.Hilight = 0;
	} else /* i == 2 */ {
	  tmpms->feel.PopupOffsetPercent = 100;
	  tmpms->feel.PopupOffsetAdd = -5;
	  tmpms->feel.f.PopupImmediately = 1;
	  tmpms->feel.f.TitleWarp = 0;
	  tmpms->look.ReliefThickness = 0;
	  tmpms->look.TitleUnderlines = 1;
	  tmpms->look.f.LongSeparators = 0;
	  tmpms->look.f.TriangleRelief = 0;
	  tmpms->look.f.Hilight = 1;
	}

	/* common settings */
	tmpms->feel.f.Animated = 0;
	FreeMenuFace(dpy, &tmpms->look.face);
	tmpms->look.face.type = SimpleMenu;
	if (tmpms->look.pStdFont && tmpms->look.pStdFont != &Scr.StdFont)
	{
	  XFreeFont(dpy, tmpms->look.pStdFont->font);
	  free(tmpms->look.pStdFont);
	}
	tmpms->look.pStdFont = &Scr.StdFont;
	gc_changed = True;
	if (tmpms->look.f.hasSideColor == 1)
	{
	  FreeColors(&tmpms->look.sideColor, 1);
	  tmpms->look.f.hasSideColor = 0;
	}
	tmpms->look.f.hasSideColor = 0;
	if (tmpms->look.sidePic)
	{
	  DestroyPicture(dpy, tmpms->look.sidePic);
	  tmpms->look.sidePic = NULL;
	}

	if (is_initialised == False)
	{
	  /* now begin the real work */
	  is_initialised = True;
	  continue;
	}
	break;

      case 3: /* Foreground */
	FreeColors(&tmpms->look.MenuColors.fore, 1);
	if (arg1)
	  tmpms->look.MenuColors.fore = GetColor(arg1);
	else
	  tmpms->look.MenuColors.fore = GetColor("black");
	gc_changed = True;
	break;

      case 4: /* Background */
	FreeColors(&tmpms->look.MenuColors.back, 1);
	if (arg1)
	  tmpms->look.MenuColors.back = GetColor(arg1);
	else
	  tmpms->look.MenuColors.back = GetColor("grey");
	gc_changed = True;
	break;

      case 5: /* Greyed */
	if (tmpms->look.f.hasStippleFore)
	  FreeColors(&tmpms->look.MenuStippleColors.fore, 1);
	if (arg1 == NULL)
	{
	  tmpms->look.f.hasStippleFore = 0;
	}
	else
	{
	  tmpms->look.MenuStippleColors.fore = GetColor(arg1);
	  tmpms->look.f.hasStippleFore = 1;
	}
	gc_changed = True;
	break;

      case 6: /* HilightBack */
	if (tmpms->look.f.hasActiveBack)
	  FreeColors(&tmpms->look.MenuActiveColors.back, 1);
	if (arg1 == NULL)
	{
	  tmpms->look.f.hasActiveBack = 0;
	}
	else
	{
	  tmpms->look.MenuActiveColors.back = GetColor(arg1);
	  tmpms->look.f.hasActiveBack = 1;
	}
	tmpms->look.f.Hilight = 1;
	gc_changed = True;
	break;

      case 7: /* HilightBackOff */
	tmpms->look.f.Hilight = 0;
	gc_changed = True;
	break;

      case 8: /* ActiveFore */
	if (tmpms->look.f.hasActiveFore)
	  FreeColors(&tmpms->look.MenuActiveColors.fore, 1);
	if (arg1 == NULL)
	{
	  tmpms->look.f.hasActiveFore = 0;
	}
	else
	{
	  tmpms->look.MenuActiveColors.fore = GetColor(arg1);
	  tmpms->look.f.hasActiveFore = 1;
	}
	gc_changed = True;
	break;

      case 9: /* ActiveForeOff */
	tmpms->look.f.hasActiveFore = 0;
	gc_changed = True;
	break;

      case 10: /* Hilight3DThick */
	tmpms->look.ReliefThickness = 2;
	break;

      case 11: /* Hilight3DThin */
	tmpms->look.ReliefThickness = 1;
	break;

      case 12: /* Hilight3DOff */
	tmpms->look.ReliefThickness = 0;
	break;

      case 13: /* Animation */
	tmpms->feel.f.Animated = 1;
	break;

      case 14: /* AnimationOff */
	tmpms->feel.f.Animated = 0;
	break;

      case 15: /* Font */
	if (arg1 && (xfs = GetFontOrFixed(dpy, arg1)) == NULL)
	{
	  fvwm_msg(ERR,"NewMenuStyle",
		   "Couldn't load font '%s' or 'fixed'\n", arg1);
	  break;
	}
	if (tmpms->look.pStdFont && tmpms->look.pStdFont != &Scr.StdFont)
	{
	  if (tmpms->look.pStdFont->font)
	    XFreeFont(dpy, tmpms->look.pStdFont->font);
	  free(tmpms->look.pStdFont);
	}
	if (arg1 == NULL)
	{
	  /* reset to screen font */
	  tmpms->look.pStdFont = &Scr.StdFont;
	}
	else
	{
	  tmpms->look.pStdFont = (MyFont *)safemalloc(sizeof(MyFont));
	  tmpms->look.pStdFont->font = xfs;
	}
	gc_changed = True;
	break;

      case 16: /* MenuFace */
	while (args && *args != '\0' && isspace(*args))
	  args++;
	ReadMenuFace(args, &tmpms->look.face, True);
	break;

      case 17: /* PopupDelay */
	if (GetIntegerArguments(args, NULL, val, 1) == 0 || *val < 0)
	  Scr.menus.PopupDelay10ms = DEFAULT_POPUP_DELAY;
	else
	  Scr.menus.PopupDelay10ms = (*val+9)/10;
	if (!is_default_style)
	{
	  fvwm_msg(WARN, "NewMenuStyle",
		   "PopupDelay applied to style '%s' will affect all menus",
		   tmpms->name);
	}
	break;

      case 18: /* PopupOffset */
	if ((n = GetIntegerArguments(args, NULL, val, 2)) == 0)
	{
	  fvwm_msg(ERR,"NewMenuStyle",
		   "PopupOffset requires one or two arguments");
	}
	else
	{
	  tmpms->feel.PopupOffsetAdd = val[0];
	  if (n == 2 && val[1] <= 100 && val[1] >= 0)
	    tmpms->feel.PopupOffsetPercent = val[1];
	  else
	    tmpms->feel.PopupOffsetPercent = 100;
	}
	break;

      case 19: /* TitleWarp */
	tmpms->feel.f.TitleWarp = 1;
	break;

      case 20: /* TitleWarpOff */
	tmpms->feel.f.TitleWarp = 0;
	break;

      case 21: /* TitleUnderlines0 */
	tmpms->look.TitleUnderlines = 0;
	break;

      case 22: /* TitleUnderlines1 */
	tmpms->look.TitleUnderlines = 1;
	break;

      case 23: /* TitleUnderlines2 */
	tmpms->look.TitleUnderlines = 2;
	break;

      case 24: /* SeparatorsLong */
	tmpms->look.f.LongSeparators = 1;
	break;

      case 25: /* SeparatorsShort */
	tmpms->look.f.LongSeparators = 0;
	break;

      case 26: /* TrianglesSolid */
	tmpms->look.f.TriangleRelief = 0;
	break;

      case 27: /* TrianglesRelief */
	tmpms->look.f.TriangleRelief = 1;
	break;

      case 28: /* PopupImmediately */
	tmpms->feel.f.PopupImmediately = 1;
	break;

      case 29: /* PopupDelayed */
	tmpms->feel.f.PopupImmediately = 0;
	break;

      case 30: /* DoubleClickTime */
	if (GetIntegerArguments(args, NULL, val, 1) == 0 || *val < 0)
	  Scr.menus.DoubleClickTime = DEFAULT_MENU_CLICKTIME;
	else
	  Scr.menus.DoubleClickTime = *val;
	if (!is_default_style)
	{
	  fvwm_msg(WARN, "NewMenuStyle",
		   "DoubleClickTime for style '%s' will affect all menus",
		   tmpms->name);
	}
	break;

      case 31: /* SidePic */
	if (tmpms->look.sidePic)
	{
	  DestroyPicture(dpy, tmpms->look.sidePic);
	  tmpms->look.sidePic = NULL;
	}
	if (arg1)
	{
	    tmpms->look.sidePic = CachePicture(dpy, Scr.Root, NULL,
					       arg1, Scr.ColorLimit);
	  if (!tmpms->look.sidePic)
	    fvwm_msg(WARN, "NewMenuStyle", "Couldn't find pixmap %s", arg1);
	}
	break;

      case 32: /* SideColor */
	if (tmpms->look.f.hasSideColor == 1)
	{
	  FreeColors(&tmpms->look.sideColor, 1);
	  tmpms->look.f.hasSideColor = 0;
	}
	if (arg1)
	{
	  tmpms->look.sideColor = GetColor(arg1);
	  tmpms->look.f.hasSideColor = 1;
	}
	break;

      case 33: /* PopupAsRootmenu */
	tmpms->feel.f.PopupAsRootmenu = 1;
	break;

      case 34: /* PopupAsSubmenu */
	tmpms->feel.f.PopupAsRootmenu = 0;
	break;

#if 0
      case 35: /* PositionHints */
	break;
#endif

      default:
	fvwm_msg(ERR,"NewMenuStyle", "unknown option '%s'", option);
	break;
      } /* switch */

      if (option)
	{
	  free(option);
	  option = NULL;
	}
      free(optstring);
      optstring = NULL;
      if (arg1)
	{
	  free(arg1);
	  arg1 = NULL;
	}
    } /* while */

  if (gc_changed)
    {
      UpdateMenuStyle(tmpms);
    } /* if (gc_changed) */

  if(Scr.menus.DefaultStyle == NULL)
    {
      /* First MenuStyle MUST be the default style */
      Scr.menus.DefaultStyle = tmpms;
      tmpms->next = NULL;
    }
  else if (ms)
    {
      /* copy our new menu face over the old one */
      memcpy(ms, tmpms, sizeof(MenuStyle));
      free(tmpms);
    }
  else
    {
      MenuStyle *before = Scr.menus.DefaultStyle;

      /* add a new menu face to list */
      tmpms->next = NULL;
      while(before->next)
	before = before->next;
      before->next = tmpms;
    }

  MakeMenus();

  return;
}

static void OldMenuStyle(F_CMD_ARGS)
{
  char *buffer, *rest;
  char *fore, *back, *stipple, *font, *style, *animated;

  rest = GetNextToken(action,&fore);
  rest = GetNextToken(rest,&back);
  rest = GetNextToken(rest,&stipple);
  rest = GetNextToken(rest,&font);
  rest = GetNextToken(rest,&style);
  rest = GetNextToken(rest,&animated);

  if(!fore || !back || !stipple || !font || !style)
    {
      fvwm_msg(ERR,"OldMenuStyle", "error in %s style specification", action);
    }
  else
    {
      buffer = (char *)safemalloc(strlen(action) + 100);
      sprintf(buffer,
	      "* %s, Foreground %s, Background %s, Greyed %s, Font %s, %s",
	      style, fore, back, stipple, font,
	      (animated && StrEquals(animated, "anim")) ?
	        "Animation" : "AnimationOff");
      NewMenuStyle(eventp, w, tmp_win, context, buffer, Module);
      free(buffer);
    }

  if(fore)
    free(fore);
  if(back)
    free(back);
  if(stipple)
    free(stipple);
  if(font)
    free(font);
  if(style)
    free(style);
  if(animated)
    free(animated);
}


void SetMenuStyle(F_CMD_ARGS)
{
  char *option;

  GetNextOption(SkipNTokens(action, 1), &option);
  if (option == NULL || GetMenuStyleIndex(option) != -1)
    NewMenuStyle(eventp, w, tmp_win, context, action, Module);
  else
    OldMenuStyle(eventp, w, tmp_win, context, action, Module);
  if (option)
    free(option);
  return;
}


void ChangeMenuStyle(F_CMD_ARGS)
{
  char *name = NULL, *menuname = NULL;
  MenuStyle *ms = NULL;
  MenuRoot *mr = NULL;


  action = GetNextToken(action,&name);
  if (name == NULL)
  {
    fvwm_msg(ERR,"ChangeMenuStyle", "needs at least two parameters");
    return;
  }

  ms = FindMenuStyle(name);
  if(ms == NULL)
  {
    fvwm_msg(ERR,"ChangeMenuStyle", "cannot find style %s", name);
    free(name);
    return;
  }
  free(name);

  action = GetNextToken(action,&menuname);
  while(menuname && *menuname)
  {
    mr = FindPopup(menuname);
    if(mr == NULL)
    {
      fvwm_msg(ERR,"ChangeMenuStyle", "cannot find menu %s", menuname);
      free(menuname);
      break;
    }
    mr->ms = ms;
    MakeMenu(mr);
    free(menuname);
    action = GetNextToken(action,&menuname);
  }
}


Boolean ReadButtonFace(char *s, ButtonFace *bf, int button, int verbose);
void FreeButtonFace(Display *dpy, ButtonFace *bf);

#ifdef BORDERSTYLE
/****************************************************************************
 *
 *  Sets the border style (veliaa@rpi.edu)
 *
 ****************************************************************************/
void SetBorderStyle(F_CMD_ARGS)
{
    char *parm = NULL, *prev = action;
#ifdef USEDECOR
    FvwmDecor *fl = cur_decor ? cur_decor : &Scr.DefaultDecor;
#else
    FvwmDecor *fl = &Scr.DefaultDecor;
#endif

    action = GetNextToken(action, &parm);
    while (parm)
    {
	if (StrEquals(parm,"active") || StrEquals(parm,"inactive"))
	{
	    int len;
	    char *end, *tmp;
	    ButtonFace tmpbf, *bf;
	    tmpbf.style = SimpleButton;
#ifdef MULTISTYLE
	    tmpbf.next = NULL;
#endif
#ifdef MINI_ICONS
	    tmpbf.u.p = NULL;
#endif
	    if (StrEquals(parm,"active"))
		bf = &fl->BorderStyle.active;
	    else
		bf = &fl->BorderStyle.inactive;
	    while (isspace(*action)) ++action;
	    if (*action != '(') {
		if (!*action) {
		    fvwm_msg(ERR,"SetBorderStyle",
			     "error in %s border specification", parm);
		    free(parm);
		    return;
		}
		free(parm);
		while (isspace(*action)) ++action;
		if (ReadButtonFace(action, &tmpbf,-1,True)) {
		    FreeButtonFace(dpy, bf);
		    *bf = tmpbf;
		}
		break;
	    }
	    end = strchr(++action, ')');
	    if (!end) {
		fvwm_msg(ERR,"SetBorderStyle",
			 "error in %s border specification", parm);
		free(parm);
		return;
	    }
	    len = end - action + 1;
	    tmp = safemalloc(len);
	    strncpy(tmp, action, len - 1);
	    tmp[len - 1] = 0;
	    ReadButtonFace(tmp, bf,-1,True);
	    free(tmp);
	    action = end + 1;
	}
	else if (strcmp(parm,"--")==0) {
	    if (ReadButtonFace(prev, &fl->BorderStyle.active,-1,True))
		ReadButtonFace(prev, &fl->BorderStyle.inactive,-1,False);
	    free(parm);
	    break;
	} else {
	    ButtonFace tmpbf;
	    tmpbf.style = SimpleButton;
#ifdef MULTISTYLE
	    tmpbf.next = NULL;
#endif
#ifdef MINI_ICONS
	    tmpbf.u.p = NULL;
#endif
	    if (ReadButtonFace(prev, &tmpbf,-1,True)) {
		FreeButtonFace(dpy,&fl->BorderStyle.active);
		fl->BorderStyle.active = tmpbf;
		ReadButtonFace(prev, &fl->BorderStyle.inactive,-1,False);
	    }
	    free(parm);
	    break;
	}
	free(parm);
	prev = action;
	action = GetNextToken(action,&parm);
    }
}
#endif

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

	    fl->WindowFont.y = fl->WindowFont.font->ascent
		+ (height - (fl->WindowFont.font->ascent
			     + fl->WindowFont.font->descent + 3)) / 2;
	    if (fl->WindowFont.y < fl->WindowFont.font->ascent)
		fl->WindowFont.y = fl->WindowFont.font->ascent;

	    tmp = Scr.FvwmRoot.next;
	    hi = Scr.Hilite;
	    while(tmp)
	    {
		if (!(tmp->flags & TITLE)
#ifdef USEDECOR
		    || (tmp->fl != fl)
#endif
		    ) {
		    tmp = tmp->next;
		    continue;
		}
		x = tmp->frame_x;
		y = tmp->frame_y;
		w = tmp->frame_width;
		h = tmp->frame_height-extra_height;
		tmp->frame_x = 0;
		tmp->frame_y = 0;
		tmp->frame_height = 0;
		tmp->frame_width = 0;
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
  MenuStyle *ms;
  int wid;
  int hei;

  Scr.StdFont.y = Scr.StdFont.font->ascent;
  Scr.StdFont.height = Scr.StdFont.font->ascent + Scr.StdFont.font->descent;

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

  gcv.foreground = Scr.StdRelief.fore;
  gcv.background = Scr.StdRelief.back;
  if(Scr.StdReliefGC)
    XChangeGC(dpy, Scr.StdReliefGC, gcm, &gcv);
  else
    Scr.StdReliefGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  gcv.foreground = Scr.StdRelief.back;
  gcv.background = Scr.StdRelief.fore;
  if(Scr.StdShadowGC)
    XChangeGC(dpy, Scr.StdShadowGC, gcm, &gcv);
  else
    Scr.StdShadowGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  /* update the geometry window for move/resize */
  if(Scr.SizeWindow != None)
  {
    XSetWindowBackground(dpy, Scr.SizeWindow, Scr.StdColors.back);
    Scr.SizeStringWidth = XTextWidth(Scr.StdFont.font, " +8888 x +8888 ", 15);
    wid = Scr.SizeStringWidth + 2 * SIZE_HINDENT;
    hei = Scr.StdFont.height + 2 * SIZE_VINDENT;
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
    Scr.IconFont.font = Scr.StdFont.font;
    ApplyIconFont();
  }

  if (Scr.flags.has_window_font == 0)
  {
    Scr.DefaultDecor.WindowFont.font = Scr.StdFont.font;
    ApplyWindowFont(&Scr.DefaultDecor);
  }

  for (ms = Scr.menus.DefaultStyle; ms; ms = ms->next)
  {
    if (ms->look.pStdFont == &Scr.StdFont)
      ms->look.EntryHeight = Scr.StdFont.height + HEIGHT_EXTRA;
    UpdateMenuStyle(ms);
  }
  MakeMenus();
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
    }
  free(fore);
  free(back);

  ApplyDefaultFontAndColors();
}

void LoadDefaultFont(F_CMD_ARGS)
{
  char *font;
  XFontStruct *xfs = NULL;

  action = GetNextToken(action,&font);
  if (!font)
  {
    /* Try 'fixed' */
    font = strdup("");
  }

  if ((xfs = GetFontOrFixed(dpy, font)) == NULL)
  {
    fvwm_msg(ERR,"SetDefaultFont","Couldn't load font '%s' or 'fixed'\n",
	     font);
    free(font);
    if (Scr.StdFont.font == NULL)
      exit(1);
    else
      return;
  }
  free(font);
  if (Scr.StdFont.font)
    XFreeFont(dpy, Scr.StdFont.font);
  Scr.StdFont.font = xfs;

  ApplyDefaultFontAndColors();
}

static void ApplyIconFont(void)
{
  FvwmWindow *tmp;

  Scr.IconFont.height = Scr.IconFont.font->ascent+Scr.IconFont.font->descent;
  Scr.IconFont.y = Scr.IconFont.font->ascent;

  tmp = Scr.FvwmRoot.next;
  while(tmp)
  {
    RedoIconName(tmp);

    if(tmp->flags& ICONIFIED)
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

  action = GetNextToken(action,&font);
  if (!font)
  {
    if (Scr.flags.has_icon_font == 1)
    {
      /* reset to default font */
      XFreeFont(dpy, Scr.IconFont.font);
      Scr.flags.has_icon_font = 0;
    }
    Scr.IconFont.font = Scr.StdFont.font;
    ApplyIconFont();
    return;
  }

  if ((newfont = GetFontOrFixed(dpy, font)))
  {
    if (Scr.IconFont.font && Scr.flags.has_icon_font == 1)
      XFreeFont(dpy, Scr.IconFont.font);
    Scr.flags.has_icon_font = 1;
    Scr.IconFont.font = newfont;
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
  FvwmWindow *tmp,*hi;
  int x,y,w,h,extra_height;

  fl->WindowFont.height =
    fl->WindowFont.font->ascent+fl->WindowFont.font->descent;
  fl->WindowFont.y = fl->WindowFont.font->ascent;
  extra_height = fl->TitleHeight;
  fl->TitleHeight=fl->WindowFont.font->ascent+fl->WindowFont.font->descent+3;
  extra_height -= fl->TitleHeight;

  tmp = Scr.FvwmRoot.next;
  hi = Scr.Hilite;
  while(tmp)
  {
    if (!(tmp->flags & TITLE)
#ifdef USEDECOR
	|| (tmp->fl != fl)
#endif
      )
    {
      tmp = tmp->next;
      continue;
    }
    x = tmp->frame_x;
    y = tmp->frame_y;
    w = tmp->frame_width;
    h = tmp->frame_height-extra_height;
    tmp->frame_x = 0;
    tmp->frame_y = 0;
    tmp->frame_height = 0;
    tmp->frame_width = 0;
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
      fl->WindowFont.font = Scr.StdFont.font;
      ApplyWindowFont(&Scr.DefaultDecor);
    }
    return;
  }

  if ((newfont = GetFontOrFixed(dpy, font)))
  {
    if (fl->WindowFont.font &&
	(fl != &Scr.DefaultDecor || Scr.flags.has_window_font == 1))
      XFreeFont(dpy, fl->WindowFont.font);
    if (fl == &Scr.DefaultDecor)
      Scr.flags.has_window_font = 1;
    fl->WindowFont.font = newfont;
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

    if (sscanf(s, "%s%n", style, &offset) < 1) {
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
	    int i, num_coords, num;
	    struct vector_coords *vc = &bf->vector;

	    /* get number of points */
	    if (strncasecmp(style,"Vector",6)==0) {
		num = sscanf(s,"%d%n",&num_coords,&offset);
		s += offset;
	    } else
		num = sscanf(style,"%d",&num_coords);

	    if((num != 1)||(num_coords>20)||(num_coords<2))
	    {
		if(verbose)fvwm_msg(ERR,"ReadButtonFace",
				    "Bad button style (2) in line: %s",action);
		return False;
	    }

	    vc->num = num_coords;

	    /* get the points */
	    for(i = 0; i < vc->num; ++i)
	    {
		/* X x Y @ line_style */
		num = sscanf(s,"%dx%d@%d%n",&vc->x[i],&vc->y[i],
			     &vc->line_style[i], &offset);
		if(num != 3)
		{
		    if(verbose)fvwm_msg(ERR,"ReadButtonFace",
					"Bad button style (3) in line %s",
					action);
		    return False;
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
		if (nsegs < 1) nsegs = 1;
		if (nsegs > 128) nsegs = 128;
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

	    if (npixels < 2) npixels = 2;
	    if (npixels > 128) npixels = 128;

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
	    bf->u.p = CachePicture(dpy, Scr.Root, NULL,
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

    while(isspace(*s))++s;
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
    while(isspace(*s))++s;
    if ('(' == *s) {
	int len;
	pstyle = 1;
	if (!(end = strchr(++s, ')'))) {
	    fvwm_msg(ERR,"ReadTitleButton",
		     "missing parenthesis: %s", s);
	    return NULL;
	}
	while (isspace(*s)) ++s;
	len = end - s + 1;
	spec = safemalloc(len);
	strncpy(spec, s, len - 1);
	spec[len - 1] = 0;
    } else
	spec = s;

    while(isspace(*spec))++spec;
    /* setup temporary in case button read fails */
    tmpbf.style = SimpleButton;
#ifdef MULTISTYLE
    tmpbf.next = NULL;
#endif
#ifdef MINI_ICONS
    tmpbf.u.p = NULL;
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
	while(isspace(*end))++end;
    }
    return end;
}

static void FreeMenuFace(Display *dpy, MenuFace *mf)
{
  switch (mf->type)
  {
#ifdef GRADIENT_BUTTONS
  case HGradMenu:
  case VGradMenu:
  case DGradMenu:
    /* - should we check visual is not TrueColor before doing this?
     *
     *            XFreeColors(dpy, PictureCMap,
     *                        ms->u.grad.pixels, ms->u.grad.npixels,
     *                        AllPlanes); */
    free(mf->u.grad.pixels);
    mf->u.grad.pixels = NULL;
    break;
#endif

#ifdef PIXMAP_BUTTONS
  case PixmapMenu:
  case TiledPixmapMenu:
    if (mf->u.p)
      DestroyPicture(dpy, mf->u.p);
    mf->u.p = NULL;
    break;
#endif
  case SolidMenu:
    FreeColors(&mf->u.back, 1);
  default:
    break;
  }
  mf->type = SimpleMenu;
}

/*****************************************************************************
 *
 * Reads a menu face line into a structure (veliaa@rpi.edu)
 *
 ****************************************************************************/
static Boolean ReadMenuFace(char *s, MenuFace *mf, int verbose)
{
  char *style;
  char *token;
  char *action = s;

  s = GetNextToken(s, &style);
  if (style && strncasecmp(style, "--", 2) == 0)
  {
    free(style);
    return True;
  }

  FreeMenuFace(dpy, mf);
  mf->type = SimpleMenu;

  /* determine menu style */
  if (!style)
    return True;
  else if (StrEquals(style,"Solid"))
  {
    s = GetNextToken(s, &token);
    if (token)
    {
      mf->type = SolidMenu;
      mf->u.back = GetColor(token);
      free(token);
    }
    else
    {
      if(verbose)
	fvwm_msg(ERR, "ReadMenuFace", "no color given for Solid face type: %s",
		 action);
      free(style);
      return False;
    }
  }

#ifdef GRADIENT_BUTTONS
  else if (StrEquals(style,"HGradient") || StrEquals(style, "VGradient") ||
	   StrEquals(style,"DGradient") || StrEquals(style, "BGradient"))
  {
    char *item, **s_colors;
    int npixels, nsegs, i, sum, perc[128];
    Pixel *pixels;
    char gtype = style[0];

    s = GetNextToken(s, &item);
    if (!item)
    {
      if(verbose)
	fvwm_msg(ERR, "ReadMenuFace",
		 "expected number of colors to allocate in gradient");
      free(style);
      return False;
    }
    npixels = atoi(item);
    free(item);

    s = GetNextToken(s, &item);
    if (!item)
    {
      if(verbose)
	fvwm_msg(ERR, "ReadMenuFace", "incomplete gradient style");
      free(style);
      return False;
    }

    if (!(isdigit(*item)))
    {
      s_colors = (char **)safemalloc(sizeof(char *) * 2);
      nsegs = 1;
      s_colors[0] = item;
      s = GetNextToken(s, &s_colors[1]);
      if (!s_colors[1])
      {
	if(verbose)
	  fvwm_msg(ERR, "ReadMenuFace", "incomplete gradient style");
	free(s_colors);
	free(item);
	free(style);
	return False;
      }
      perc[0] = 100;
    }
    else
    {
      nsegs = atoi(item);
      free(item);
      if (nsegs < 1)
	nsegs = 1;
      if (nsegs > 128)
	nsegs = 128;
      s_colors = (char **)safemalloc(sizeof(char *) * (nsegs + 1));
      for (i = 0; i <= nsegs; ++i)
      {
	s = GetNextToken(s, &s_colors[i]);
	if (i < nsegs)
	{
	  s = GetNextToken(s, &item);
	  perc[i] = (item) ? atoi(item) : 0;
	  if (item)
	    free(item);
	}
      }
    }

    for (i = 0, sum = 0; i < nsegs; ++i)
      sum += perc[i];

    if (sum != 100)
    {
      if(verbose)
	fvwm_msg(ERR,"ReadMenuFace", "multi gradient lenghts must sum to 100");
      for (i = 0; i <= nsegs; ++i)
	if (s_colors[i])
	  free(s_colors[i]);
      free(style);
      free(s_colors);
      return False;
    }

    if (npixels < 2)
      npixels = 2;
    if (npixels > 128)
      npixels = 128;

    pixels = AllocNonlinearGradient(s_colors, perc, nsegs, npixels);
    for (i = 0; i <= nsegs; ++i)
      if (s_colors[i])
	free(s_colors[i]);
    free(s_colors);

    if (!pixels)
    {
      if(verbose)
	fvwm_msg(ERR, "ReadMenuFace", "couldn't create gradient");
      free(style);
      return False;
    }

    mf->u.grad.pixels = pixels;
    mf->u.grad.npixels = npixels;

    switch (gtype)
    {
    case 'h':
    case 'H':
      mf->type = HGradMenu;
      break;
    case 'v':
    case 'V':
      mf->type = VGradMenu;
      break;
    case 'd':
    case 'D':
      mf->type = DGradMenu;
      break;
    default:
      mf->type = BGradMenu;
      break;
    }
  }
#endif /* GRADIENT_BUTTONS */

#ifdef PIXMAP_BUTTONS
  else if (StrEquals(style,"Pixmap") || StrEquals(style,"TiledPixmap"))
  {
    s = GetNextToken(s, &token);
    if (token)
    {
      mf->u.p = CachePicture(dpy, Scr.Root, NULL,
			     token, Scr.ColorLimit);
      if (mf->u.p == NULL)
      {
	if(verbose)
	  fvwm_msg(ERR, "ReadMenuFace", "couldn't load pixmap %s", token);
	free(token);
	free(style);
	return False;
      }
      free(token);
      mf->type = (StrEquals(style,"TiledPixmap")) ?
	TiledPixmapMenu : PixmapMenu;
    }
    else
    {
      if(verbose)
	fvwm_msg(ERR, "ReadMenuFace", "missing pixmap name for style %s",
		 style);
	free(style);
	return False;
    }
  }
#endif /* PIXMAP_BUTTONS */

  else
  {
    if(verbose)
      fvwm_msg(ERR, "ReadMenuFace", "unknown style %s: %s", style, action);
    free(style);
    return False;
  }

  free(style);
  return True;
}

#ifdef USEDECOR
/*****************************************************************************
 *
 * Diverts a style definition to an FvwmDecor structure (veliaa@rpi.edu)
 *
 ****************************************************************************/
void AddToDecor(FvwmDecor *fl, char *s)
{
    if (!s) return;
    while (*s&&isspace(*s))++s;
    if (!*s) return;
    cur_decor = fl;
    ExecuteFunction(s,NULL,&Event,C_ROOT,-1);
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
    if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
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
    extra_height = (tmp_win->flags & TITLE) ?
      (old_height - tmp_win->fl->TitleHeight) : 0;
    x = tmp_win->frame_x;
    y = tmp_win->frame_y;
    width = tmp_win->frame_width;
    height = tmp_win->frame_height - extra_height;
    tmp_win->frame_x = 0;
    tmp_win->frame_y = 0;
    tmp_win->frame_height = 0;
    tmp_win->frame_width = 0;
    SetupFrame(tmp_win,x,y,width,height,True,False);
    SetBorder(tmp_win,Scr.Hilite == tmp_win,True,True,None);
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
			      C_WINDOW,*Module);
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
	for (fl = &Scr.DefaultDecor; fl->next; fl = fl->next);
	fl->next = found;
    } else
	free(item);

    if (found) {
	AddToDecor(found, s);

	/* Set + state to last decor */
	last_decor = found;
	last_menu = NULL;
	last_func = NULL;
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
char *CreateFlagString(char *string, char **restptr)
{
  char *retval;
  char *c;
  char *start;
  char closeopt;
  int length;

  c = string;
  while (isspace(*c) && (*c != 0))
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
	fvwm_msg(ERR, "CreateConditionMask",
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
void FreeConditionMask(WindowConditionMask *mask)
{
  if (mask->needsName)
    free(mask->name);
  else if (mask->needsNotName)
    free(mask->name - 1);
}

/* Assign the default values for the window mask */
void DefaultConditionMask(WindowConditionMask *mask)
{
  mask->name = NULL;
  mask->needsCurrentDesk = 0;
  mask->needsCurrentPage = 0;
  mask->needsName = 0;
  mask->needsNotName = 0;
  mask->useCirculateHit = 0;
  mask->useCirculateHitIcon = 0;
  mask->onFlags = 0;
  mask->offFlags = 0;
  mask->layer = -2; /* -2  means no layer condition, -1 means current */
}

/**********************************************************************
 * Note that this function allocates the name field of the mask struct.
 * FreeConditionMask must be called for the mask when the mask is discarded.
 **********************************************************************/
void CreateConditionMask(char *flags, WindowConditionMask *mask)
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
      mask->onFlags |= ICONIFIED;
    else if(StrEquals(condition,"!Iconic"))
      mask->offFlags |= ICONIFIED;
    else if(StrEquals(condition,"Visible"))
      mask->onFlags |= VISIBLE;
    else if(StrEquals(condition,"!Visible"))
      mask->offFlags |= VISIBLE;
    else if(StrEquals(condition,"Sticky"))
      mask->onFlags |= STICKY;
    else if(StrEquals(condition,"!Sticky"))
      mask->offFlags |= STICKY;
    else if(StrEquals(condition,"Maximized"))
      mask->onFlags |= MAXIMIZED;
    else if(StrEquals(condition,"!Maximized"))
      mask->offFlags |= MAXIMIZED;
    else if(StrEquals(condition,"Transient"))
      mask->onFlags |= TRANSIENT;
    else if(StrEquals(condition,"!Transient"))
      mask->offFlags |= TRANSIENT;
    else if(StrEquals(condition,"CurrentDesk"))
      mask->needsCurrentDesk = 1;
    else if(StrEquals(condition,"CurrentPage"))
    {
      mask->needsCurrentDesk = 1;
      mask->needsCurrentPage = 1;
    }
    else if(StrEquals(condition,"CurrentPageAnyDesk") ||
	    StrEquals(condition,"CurrentScreen"))
      mask->needsCurrentPage = 1;
    else if(StrEquals(condition,"CirculateHit"))
      mask->useCirculateHit = 1;
    else if(StrEquals(condition,"CirculateHitIcon"))
      mask->useCirculateHitIcon = 1;
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
    else if(!mask->needsName && !mask->needsNotName)
    {
      /* only 1st name to avoid mem leak */
      mask->name = condition;
      condition = NULL;
      if (mask->name[0] == '!')
      {
	mask->needsNotName = 1;
	mask->name++;
      }
      else
	mask->needsName = 1;
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
Bool MatchesConditionMask(FvwmWindow *fw, WindowConditionMask *mask)
{
  Bool fMatchesName;
  Bool fMatchesIconName;
  Bool fMatchesClass;
  Bool fMatchesResource;
  Bool fMatches;

  if ((mask->onFlags & fw->flags) != mask->onFlags)
    return 0;

  if ((mask->offFlags & fw->flags) != 0)
    return 0;

  if (!mask->useCirculateHit && (fw->flags & CirculateSkip))
    return 0;

  /* This logic looks terribly wrong to me, but it was this way before so I
   * did not change it (domivogt (24-Dec-1998)) */
  if (!mask->useCirculateHitIcon && fw->flags & ICONIFIED &&
      fw->flags & CirculateSkipIcon)
    return 0;

  if (fw->flags & ICONIFIED && fw->flags & TRANSIENT &&
      fw->tmpflags.IconifiedByParent)
    return 0;

  if (mask->needsCurrentDesk && fw->Desk != Scr.CurrentDesk)
    return 0;

  if (mask->needsCurrentPage && !(fw->frame_x < Scr.MyDisplayWidth &&
				  fw->frame_y < Scr.MyDisplayHeight &&
				  fw->frame_x + fw->frame_width > 0 &&
				  fw->frame_y + fw->frame_height > 0))
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

  if (mask->needsName && !fMatches)
    return 0;

  if (mask->needsNotName && fMatches)
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
FvwmWindow *Circulate(char *action, int Direction, char **restofline)
{
  int pass = 0;
  FvwmWindow *fw, *found = NULL;
  WindowConditionMask mask;
  char *flags;

  /* Create window mask */
  flags = CreateFlagString(action, restofline);
  DefaultConditionMask(&mask);
  if (Direction == 0) { /* override for Current [] */
    mask.useCirculateHit = 1;
    mask.useCirculateHitIcon = 1;
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
    ExecuteFunction(restofline,found,eventp,C_WINDOW,*Module);
  }

}

void NextFunc(F_CMD_ARGS)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, 1, &restofline);
  if(found && restofline)
  {
    ExecuteFunction(restofline,found,eventp,C_WINDOW,*Module);
  }

}

void NoneFunc(F_CMD_ARGS)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, 1, &restofline);
  if(!found && restofline)
  {
    ExecuteFunction(restofline,NULL,eventp,C_ROOT,*Module);
  }
}

void CurrentFunc(F_CMD_ARGS)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, 0, &restofline);
  if(found && restofline)
  {
    ExecuteFunction(restofline,found,eventp,C_WINDOW,*Module);
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
  mask.useCirculateHit = 1;
  mask.useCirculateHitIcon = 1;

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
       ExecuteFunction(restofline,g[i],eventp,C_WINDOW,*Module);
    }

  free (g);
  FreeConditionMask(&mask);
}

static void GetDirectionReference(FvwmWindow *w, int *x, int *y)
{
  if ((w->flags & ICONIFIED) != 0)
  {
    *x = w->icon_x_loc + w->icon_w_width / 2;
    *y = w->icon_y_loc + w->icon_w_height / 2;
  }
  else
  {
    *x = w->frame_x + w->frame_width / 2;
    *y = w->frame_y + w->frame_height / 2;
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
    ExecuteFunction(restofline, best_window, eventp, C_WINDOW, *Module);

  FreeConditionMask(&mask);
}

/* A very simple function, but handy if you want to call
 * complex functions from root context without selecting a window
 * for every single function in it. */
void PickFunc(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
    return;

  ExecuteFunction(action, tmp_win, eventp, C_WINDOW, *Module);
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
    mask.useCirculateHit = True;
    mask.useCirculateHitIcon = True;

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
        ExecuteFunction(action,t,eventp,C_WINDOW,*Module);
      break;
  }

  /* Tidy up */
  if (use_condition)
  {
    FreeConditionMask(&mask);
  }
}


void module_zapper(F_CMD_ARGS)
{
  char *condition;

  GetNextToken(action,&condition);
  if (!condition)
    return;
  KillModuleByName(condition);
  free(condition);
}

/***********************************************************************
 *
 *  Procedure:
 *	Reborder - Removes fvwm border windows
 *
 ************************************************************************/
void Recapture(F_CMD_ARGS)
{
  XEvent event;

  /* Wow, this grabbing speeds up recapture tremendously! I think that is the
   * solution for this weird -blackout option. */
  MyXGrabServer(dpy);
  GrabEm(WAIT);
  BlackoutScreen(); /* if they want to hide the recapture */
  CaptureAllWindows();
  UnBlackoutScreen();
  /* Throw away queued up events. We don't want user input during a
   * recapture. The window the user clicks in might disapper at the very same
   * moment and the click goes through to the root window. Not goot */
  while (XCheckMaskEvent(dpy, ButtonPressMask|ButtonReleaseMask|
			 ButtonMotionMask|PointerMotionMask|EnterWindowMask|
			 LeaveWindowMask, &event) != False)
    ;
  UngrabEm();
  MyXUngrabServer(dpy);
  XSync(dpy, 0);
}

void SetGlobalOptions(F_CMD_ARGS)
{
  char *opt;

  /* fvwm_msg(DBG,"SetGlobalOptions","init action == '%s'\n",action); */
  for (action = GetNextOption(action, &opt); opt;
       action = GetNextOption(action, &opt))
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
    else
      fvwm_msg(ERR,"SetGlobalOptions","Unknown Global Option '%s'",opt);
    if (opt) /* should never be null, but checking anyways... */
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
void SetColorLimit(F_CMD_ARGS)
{
  int val;

  if (Scr.d_depth > 20) {               /* if more than 20 bit color */
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

#ifdef WINDOWSHADE
/* set the number or size of shade animation steps, N => steps, Np => pixels */
void setShadeAnim(F_CMD_ARGS)
{
  int n = 0;
  int val;
  char *opt;

  action = GetNextToken(action, &opt);
  if (opt) {
    if ((val = strlen(opt))) {
      if (opt[val - 1] == 'p') {
        opt[val - 1] = '\0';
        n = sscanf(opt, "%d", &val);
        val = -val; /* negative values mean pixels */
      } else {
        n = sscanf(opt, "%d", &val);
      }
      if (n == 1) {
        shade_anim_steps = val;
        return;
      }
    }
  }

  /* doh! something is wrong */
  fvwm_msg(ERR,"setShadeAnim","WindowShadeAnimate requires 1 argument");
  return;
}
#endif /* WINDOWSHADE */

void change_layer(F_CMD_ARGS)
{
  int n, layer, val[2];
  FvwmWindow *t2, *next;
  name_list styles;

  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
    return;

  if(tmp_win == NULL)
    return;

  n = GetIntegerArguments(action, NULL, val, 2);

  layer = tmp_win->layer;
  if ((n == 1) ||
      ((n == 2) && (val[0] != 0)))
    {
      layer += val[0];
    }
  else if ((n == 2) && (val[1] >= 0))
    {
      layer = val[1];
    }
  else
    {
      /* a hack: Layer 0 -1 goes back to the "default" layer */
      LookInList (tmp_win, &styles);
      layer = styles.layer;
    }

  if (layer < 0)
    {
      layer = 0;
    }

  if (layer < tmp_win->layer)
    {
      tmp_win->layer = layer;
      RaiseWindow(tmp_win);
    }
  else if (layer > tmp_win->layer)
    {
#ifndef DONT_RAISE_TRANSIENTS
      /* this could be done much more efficiently */
      for (t2 = Scr.FvwmRoot.stack_next; t2 != &Scr.FvwmRoot; t2 = next)
	{
	  next = t2->stack_next;
	  if ((t2->flags & TRANSIENT) &&
	      (t2->transientfor == tmp_win->w) &&
	      (t2 != tmp_win) &&
	      (t2->layer >= tmp_win->layer) &&
              (t2->layer < layer))
	    {
	      t2->layer = layer;
	      LowerWindow(t2);
	    }
	}
#endif
      tmp_win->layer = layer;
      LowerWindow(tmp_win);
    }
}

void SetDefaultLayers(F_CMD_ARGS)
{
  char *tok = NULL;
  int i;

  action = GetNextToken(action, &tok);

  if (tok)
    {
       i = atoi (tok);
       if (i < 0)
         {
           fvwm_msg(ERR,"DefaultLayers", "Layer must be non-negative." );
         }
       else
         {
           Scr.OnTopLayer = i;
         }
       free (tok);
       tok = NULL;
    }

  action = GetNextToken(action, &tok);

  if (tok)
    {
       i = atoi (tok);
       if (i < 0)
         {
            fvwm_msg(ERR,"DefaultLayers", "Layer must be non-negative." );
         }
       else
         {
            Scr.StaysPutLayer = i;
         }
       free (tok);
       tok = NULL;
    }
}
