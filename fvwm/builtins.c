/****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <X11/keysym.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

static Boolean ReadMenuFace(char *s, MenuFace *mf, int verbose);
static void FreeMenuFace(Display *dpy, MenuFace *mf);

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

/***********************************************************************
 *
 *  Procedure:
 *	DeferExecution - defer the execution of a function to the
 *	    next button press if the context is C_ROOT
 *
 *  Inputs:
 *      eventp  - pointer to XEvent to patch up
 *      w       - pointer to Window to patch up
 *      tmp_win - pointer to FvwmWindow Structure to patch up
 *	context	- the context in which the mouse button was pressed
 *	func	- the function to defer
 *	cursor	- the cursor to display while waiting
 *      finishEvent - ButtonRelease or ButtonPress; tells what kind of event to
 *                    terminate on.
 *
 ***********************************************************************/
int DeferExecution(XEvent *eventp, Window *w,FvwmWindow **tmp_win,
		   unsigned long *context, int cursor, int FinishEvent)

{
  int done;
  int finished = 0;
  Window dummy;
  Window original_w;

  original_w = *w;

  if((*context != C_ROOT)&&(*context != C_NO_CONTEXT)&&(tmp_win != NULL))
  {
    if((FinishEvent == ButtonPress)||((FinishEvent == ButtonRelease) &&
                                      (eventp->type != ButtonPress)))
    {
      return FALSE;
    }
  }
  if(!GrabEm(cursor))
  {
    XBell(dpy, 0);
    return True;
  }

  while (!finished)
  {
    done = 0;
    /* block until there is an event */
    XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
               ExposureMask |KeyPressMask | VisibilityChangeMask |
               ButtonMotionMask| PointerMotionMask/* | EnterWindowMask |
                                                     LeaveWindowMask*/, eventp);
    StashEventTime(eventp);

    if(eventp->type == KeyPress)
      Keyboard_shortcuts(eventp, NULL, FinishEvent);
    if(eventp->type == FinishEvent)
      finished = 1;
    if(eventp->type == ButtonPress)
    {
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
      done = 1;
    }
    if(eventp->type == ButtonRelease)
      done = 1;
    if(eventp->type == KeyPress)
      done = 1;

    if(!done)
    {
      DispatchEvent();
    }

  }


  *w = eventp->xany.window;
  if(((*w == Scr.Root)||(*w == Scr.NoFocusWin))
     && (eventp->xbutton.subwindow != (Window)0))
  {
    *w = eventp->xbutton.subwindow;
    eventp->xany.window = *w;
  }
  if (*w == Scr.Root)
  {
    *context = C_ROOT;
    XBell(dpy, 0);
    UngrabEm();
    return TRUE;
  }
  if (XFindContext (dpy, *w, FvwmContext, (caddr_t *)tmp_win) == XCNOENT)
  {
    *tmp_win = NULL;
    XBell(dpy, 0);
    UngrabEm();
    return (TRUE);
  }

  if(*w == (*tmp_win)->Parent)
    *w = (*tmp_win)->w;

  if(original_w == (*tmp_win)->Parent)
    original_w = (*tmp_win)->w;

  /* this ugly mess attempts to ensure that the release and press
   * are in the same window. */
  if((*w != original_w)&&(original_w != Scr.Root)&&
     (original_w != None)&&(original_w != Scr.NoFocusWin))
    if(!((*w == (*tmp_win)->frame)&&
         (original_w == (*tmp_win)->w)))
    {
      *context = C_ROOT;
      XBell(dpy, 0);
      UngrabEm();
      return TRUE;
    }

  *context = GetContext(*tmp_win,eventp,&dummy);

  UngrabEm();
  return FALSE;
}




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
  KeepOnTop();

  /* If the window is still not visible, make it visible! */
  if(((t->frame_x + t->frame_height)< 0)||(t->frame_y + t->frame_width < 0)||
     (t->frame_x >Scr.MyDisplayWidth)||(t->frame_y>Scr.MyDisplayHeight))
  {
    SetupFrame(t,0,0,t->frame_width, t->frame_height,False);
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
    cx = t->frame_x + t->frame_width/2 + t->bw;
    cy = t->frame_y + t->frame_height/2 + t->bw;
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
      x = t->frame_x + t->bw + warp_x;
    else
      x = t->frame_x + t->bw + (t->frame_width - 1) * warp_x / 100;
    if (y_unit != Scr.MyDisplayHeight)
      y = t->frame_y + t->bw + warp_y;
    else
      y = t->frame_y + t->bw + (t->frame_height - 1) * warp_y / 100;
  }
  if (warp_x >= 0 && warp_y >= 0) {
    XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, x, y);
  }
  RaiseWindow(t);
  KeepOnTop();

  /* If the window is still not visible, make it visible! */
  if(((t->frame_x + t->frame_height)< 0)||(t->frame_y + t->frame_width < 0)||
     (t->frame_x >Scr.MyDisplayWidth)||(t->frame_y>Scr.MyDisplayHeight))
  {
    SetupFrame(t,0,0,t->frame_width, t->frame_height,False);
    XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, 2,2);
  }
  UngrabEm();
}



/***********************************************************************
 *
 *  Procedure:
 *	(Un)Maximize a window.
 *
 ***********************************************************************/
void Maximize(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	      unsigned long context, char *action, int *Module)
{
  int new_width, new_height,new_x,new_y;
  int val1, val2, val1_unit,val2_unit,n;

  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
    return;

  if(tmp_win == NULL)
    return;

  if(check_allowed_function2(F_MAXIMIZE,tmp_win) == 0
#ifdef WINDOWSHADE
     || (tmp_win->buttons & WSHADE)
#endif
     )
  {
    XBell(dpy, 0);
    return;
  }
  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
  if(n != 2)
  {
    val1 = 100;
    val2 = 100;
    val1_unit = Scr.MyDisplayWidth;
    val2_unit = Scr.MyDisplayHeight;
  }

  if (tmp_win->flags & MAXIMIZED)
  {
    tmp_win->flags &= ~MAXIMIZED;
    SetupFrame(tmp_win, tmp_win->orig_x, tmp_win->orig_y, tmp_win->orig_wd,
               tmp_win->orig_ht,TRUE);
    SetBorder(tmp_win,True,True,True,None);
  }
  else
  {
    new_width = tmp_win->frame_width;
    new_height = tmp_win->frame_height;
    new_x = tmp_win->frame_x;
    new_y = tmp_win->frame_y;
    if(val1 >0)
    {
      new_width = val1*val1_unit/100-2;
      new_x = 0;
    }
    if(val2 >0)
    {
      new_height = val2*val2_unit/100-2;
      new_y = 0;
    }
    if((val1==0)&&(val2==0))
    {
      new_x = 0;
      new_y = 0;
      new_height = Scr.MyDisplayHeight-2;
      new_width = Scr.MyDisplayWidth-2;
    }
    tmp_win->flags |= MAXIMIZED;
    ConstrainSize (tmp_win, &new_width, &new_height, False, 0, 0);
    SetupFrame(tmp_win,new_x,new_y,new_width,new_height,TRUE);
    SetBorder(tmp_win,Scr.Hilite == tmp_win,True,True,None);
  }
}

#ifdef WINDOWSHADE
/***********************************************************************
 *
 *  WindowShade -- shades or unshades a window (veliaa@rpi.edu)
 *
 *  Args: 1 -- force shade, 2 -- force unshade  No Arg: toggle
 *
 ***********************************************************************/
void WindowShade(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		 unsigned long context, char *action, int *Module)
{
    int n = 0;

    if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
	return;

    if (!(tmp_win->flags & TITLE) || (tmp_win->flags & MAXIMIZED)) {
	XBell(dpy, 0);
	return;
    }
    while (isspace(*action))++action;
    if (isdigit(*action))
	sscanf(action,"%d",&n);

    if (((tmp_win->buttons & WSHADE)||(n==2))&&(n!=1))
    {
	tmp_win->buttons &= ~WSHADE;
	SetupFrame(tmp_win,
		   tmp_win->frame_x,
		   tmp_win->frame_y,
		   tmp_win->orig_wd,
		   tmp_win->orig_ht,
		   True);
        BroadcastPacket(M_DEWINDOWSHADE, 3,
                        tmp_win->w, tmp_win->frame, (unsigned long)tmp_win);
    }
    else
    {
	tmp_win->buttons |= WSHADE;
	SetupFrame(tmp_win,
		   tmp_win->frame_x,
		   tmp_win->frame_y,
		   tmp_win->frame_width,
		   tmp_win->title_height + tmp_win->boundary_width,
		   False);
        BroadcastPacket(M_WINDOWSHADE, 3,
                        tmp_win->w, tmp_win->frame, (unsigned long)tmp_win);
    }
}
#endif /* WINDOWSHADE */

/* For Ultrix 4.2 */
#include <sys/types.h>
#include <sys/time.h>


MenuRoot *FindPopup(char *action)
{
  char *tmp;
  MenuRoot *mr;

  GetNextToken(action,&tmp);

  if(tmp == NULL)
    return NULL;

  mr = Scr.menus.all;
  while(mr != NULL)
  {
    if(mr->name != NULL)
      if(strcasecmp(tmp,mr->name)== 0)
      {
        free(tmp);
        return mr;
      }
    mr = mr->next;
  }
  free(tmp);
  return NULL;

}



void Bell(XEvent *eventp,Window w,FvwmWindow *tmp_win,unsigned long context,
	  char *action, int *Module)
{
  XBell(dpy, 0);
}


#ifdef USEDECOR
static FvwmDecor *last_decor = NULL, *cur_decor = NULL;
#endif
MenuRoot *last_menu=NULL;
void add_item_to_menu(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,
		      char *action, int *Module)
{
  MenuRoot *mr;
  MenuRoot *mrPrior;
  char *token, *rest,*item;

#ifdef USEDECOR
  last_decor = NULL;
#endif

  rest = GetNextToken(action,&token);
  if (!token)
    return;
  mr = FollowMenuContinuations(FindPopup(token),&mrPrior);
  if(mr == NULL)
    mr = NewMenuRoot(token, False);
  last_menu = mr;

  rest = GetNextToken(rest,&item);
  AddToMenu(mr, item,rest,TRUE /* pixmap scan */, TRUE);
  if (item)
    free(item);
  /* These lines are correct! We must not release token if the string is empty.
   * It cannot be NULL! GetNextToken never returns an empty string! */
  if (*token)
    free(token);

  MakeMenu(mr);
  return;
}


void add_another_item(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,
		      char *action, int *Module)
{
#ifdef USEDECOR
  extern void AddToDecor(FvwmDecor *, char *);
#endif
  MenuRoot *mr;
  MenuRoot *mrPrior;
  char *rest,*item;

  if (last_menu != NULL) {
    mr = FollowMenuContinuations(last_menu,&mrPrior);
    if(mr == NULL)
      return;
    rest = GetNextToken(action,&item);
    AddToMenu(mr, item,rest,TRUE /* pixmap scan */, FALSE);
    if (item)
      free(item);
    MakeMenu(mr);
  }
#ifdef USEDECOR
  else if (last_decor != NULL) {
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

void destroy_menu(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                  unsigned long context,
                  char *action, int *Module)
{
  MenuRoot *mr;
  MenuRoot *mrContinuation;

  char *token, *rest;

  rest = GetNextToken(action,&token);
  if (!token)
    return;
  mr = FindPopup(token);
  free(token);
  while (mr)
  {
    mrContinuation = mr->continuation; /* save continuation before destroy */
    DestroyMenu(mr);
    mr = mrContinuation;
  }
  return;
}

void add_item_to_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,
		      char *action, int *Module)
{
  MenuRoot *mr;

  char *token, *rest,*item;

  rest = GetNextToken(action,&token);
  if (!token)
    return;
  mr = FindPopup(token);
  if(mr == NULL)
    mr = NewMenuRoot(token, True);
  last_menu = mr;
  if (token)
    free(token);
  rest = GetNextToken(rest,&item);
  AddToMenu(mr, item,rest,FALSE,FALSE);
  if (item)
    free(item);

  return;
}


void Nop_func(XEvent *eventp, Window w, FvwmWindow *tmp_win,
	      unsigned long context, char *action, int *Module)
{

}


void movecursor(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action, int *Module)
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


void iconify_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,char *action, int *Module)

{
  int val = 0;

  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT, ButtonRelease))
    return;

  GetIntegerArguments(action, NULL, &val, 1);

  if (tmp_win->flags & ICONIFIED)
  {
    if(val <=0)
      DeIconify(tmp_win);
  }
  else
  {
    if(check_allowed_function2(F_ICONIFY,tmp_win) == 0)
    {
      XBell(dpy, 0);
      return;
    }
    if(val >=0)
      Iconify(tmp_win,eventp->xbutton.x_root-5,eventp->xbutton.y_root-5);
  }
}

void raise_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context, char *action, int *Module)
{
  name_list styles;                     /* place for merged styles */

  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
    return;

  if(tmp_win)
    RaiseWindow(tmp_win);

  LookInList(tmp_win, &styles);         /* get merged styles */
  if (styles.on_flags & STAYSONTOP_FLAG) {
    tmp_win->flags |= ONTOP;
  }
  KeepOnTop();
}

void lower_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context,char *action, int *Module)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT, ButtonRelease))
    return;

  LowerWindow(tmp_win);

  tmp_win->flags &= ~ONTOP;
}

void destroy_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context, char *action, int *Module)
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

void delete_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context,char *action, int *Module)
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

void close_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context,char *action, int *Module)
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

void restart_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context, char *action, int *Module)
{
  Done(1, action);
}

void exec_setup(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                unsigned long context,char *action, int *Module)
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

void exec_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		   unsigned long context,char *action, int *Module)
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

void refresh_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context, char *action, int *Module)
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
		     (Visual *) CopyFromParent, valuemask,
		     &attributes);
  XMapWindow (dpy, w);
  XDestroyWindow (dpy, w);
  XFlush (dpy);
}


void refresh_win_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                          unsigned long context, char *action, int *Module)
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
		     (Visual *) CopyFromParent, valuemask,
		     &attributes);
  XMapWindow (dpy, w);
  XDestroyWindow (dpy, w);
  XFlush (dpy);
}


void stick_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context, char *action, int *Module)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;

  if(tmp_win->flags & STICKY)
  {
    tmp_win->flags &= ~STICKY;
  }
  else
  {
    tmp_win->flags |=STICKY;
    move_window_doit(eventp, w, tmp_win, context, "", Module, FALSE, TRUE);
  }
  BroadcastConfig(M_CONFIGURE_WINDOW,tmp_win);
  SetTitleBar(tmp_win,(Scr.Hilite==tmp_win),True);
}

void wait_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	       unsigned long context,char *action, int *Module)
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

void flip_focus_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action, int *Module)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;

  /* Reorder the window list */
  FocusOn(tmp_win,TRUE);
}


void focus_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action, int *Module)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;

  FocusOn(tmp_win,FALSE);
}


void warp_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
               unsigned long context, char *action, int *Module)
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


static void menu_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context, char *action,int *Module,
		      Bool fStaysUp)
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
    if(menu_name != NULL)
    {
      fvwm_msg(ERR,"menu_func","No such menu %s",menu_name);
      free(menu_name);
    }
    return;
  }
  if(menu_name != NULL)
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
void popup_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int *Module)
{
  menu_func(eventp, w, tmp_win, context, action, Module, False);
}

/* the function for the "Menu" command */
void staysup_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                  unsigned long context, char *action,int *Module)
{
  menu_func(eventp, w, tmp_win, context, action, Module, True);
}


void quit_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	       unsigned long context, char *action,int *Module)
{
  if (master_pid != getpid())
    kill(master_pid, SIGTERM);
  Done(0,NULL);
}

void quit_screen_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                      unsigned long context, char *action,int *Module)
{
  Done(0,NULL);
}

void echo_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
               unsigned long context, char *action,int *Module)
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

void raiselower_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action,int *Module)
{
  name_list styles;

  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
    return;
  if(tmp_win == NULL)
    return;

  if((tmp_win == Scr.LastWindowRaised)||
     (tmp_win->flags & VISIBLE))
  {
    LowerWindow(tmp_win);
    tmp_win->flags &= ~ONTOP;
  }
  else
  {
    RaiseWindow(tmp_win);
    LookInList(tmp_win, &styles);       /* get merged styles */
    if (styles.on_flags & STAYSONTOP_FLAG) {
      tmp_win->flags |= ONTOP;
    }
    KeepOnTop();
  }
}

void SetEdgeScroll(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                   unsigned long context, char *action,int* Module)
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
    Scr.flags |= EdgeWrapX;
  }
  else
  {
    Scr.flags &= ~EdgeWrapX;
  }
  if (val2 >= 1000)
  {
    val2 /= 1000;
    Scr.flags |= EdgeWrapY;
  }
  else
  {
    Scr.flags &= ~EdgeWrapY;
  }

  Scr.EdgeScrollX = val1*val1_unit/100;
  Scr.EdgeScrollY = val2*val2_unit/100;

  checkPanFrames();
}

void SetEdgeResistance(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                       unsigned long context, char *action,int* Module)
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

void SetColormapFocus(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                      unsigned long context, char *action,int* Module)
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

void SetClick(XEvent *eventp,Window w,FvwmWindow *tmp_win,
              unsigned long context, char *action,int* Module)
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


void SetSnapAttraction(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		       unsigned long context, char *action,int* Module)
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

  if(StrEquals(token,"All"))
    { Scr.SnapMode = 0; }
  if(StrEquals(token,"SameType"))
    { Scr.SnapMode = 1; }
  if(StrEquals(token,"Icons"))
    { Scr.SnapMode = 2; }
  if(StrEquals(token,"Windows"))
    { Scr.SnapMode = 3; }

  free(token);
}

void SetSnapGrid(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		 unsigned long context, char *action,int* Module)
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


void SetXOR(XEvent *eventp,Window w,FvwmWindow *tmp_win,
            unsigned long context, char *action,int* Module)
{
  int val;
  XGCValues gcv;
  unsigned long gcm;

  if(GetIntegerArguments(action, NULL, &val, 1) != 1)
  {
    fvwm_msg(ERR,"SetXOR","XORValue requires 1 argument");
    return;
  }

  gcm = GCFunction|GCLineWidth|GCForeground|GCSubwindowMode;
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
  gcv.subwindow_mode = IncludeInferiors;
  if (Scr.DrawGC)
    XFreeGC(dpy, Scr.DrawGC);
  Scr.DrawGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);
}

void SetOpaque(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	       unsigned long context, char *action,int* Module)
{
  int val;

  if(GetIntegerArguments(action, NULL, &val, 1) != 1)
  {
    fvwm_msg(ERR,"SetOpaque","OpaqueMoveSize requires 1 argument");
    return;
  }

  Scr.OpaqueSize = val;
}


void SetDeskSize(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                 unsigned long context, char *action,int* Module)
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

#ifdef XPM
char *PixmapPath = FVWM_ICONDIR;
#else
char *PixmapPath = "";
#endif
void setPixmapPath(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                   unsigned long context, char *action,int* Module)
{
#ifdef XPM
  static char *ptemp = NULL;
  char *tmp;

  if(ptemp == NULL)
    ptemp = PixmapPath;

  if((PixmapPath != ptemp)&&(PixmapPath != NULL))
    free(PixmapPath);
  tmp = stripcpy(action);
  PixmapPath = envDupExpand(tmp, 0);
  free(tmp);
#else
  fvwm_msg(ERR, "setPixmapPath",
           "XPM support has not been included in this version of Fvwm2.");
#endif
}

char *IconPath = FVWM_ICONDIR;
void setIconPath(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                 unsigned long context, char *action,int* Module)
{
  static char *ptemp = NULL;
  char *tmp;

  if(ptemp == NULL)
    ptemp = IconPath;

  if((IconPath != ptemp)&&(IconPath != NULL))
    free(IconPath);
  tmp = stripcpy(action);
  IconPath = envDupExpand(tmp, 0);
  free(tmp);
}

char *ModulePath = FVWM_MODULEDIR;

void setModulePath(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                   unsigned long context, char *action,int* Module)
{
    static int need_to_free = 0;
    char *tmp;

    if ( need_to_free && ModulePath != NULL )
	free(ModulePath);

    tmp = stripcpy(action);
    ModulePath = envDupExpand(tmp, 0);
    need_to_free = 1;
    free(tmp);
}


void SetHiColor(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int* Module)
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
    if(hifore != NULL)
    {
	fl->HiColors.fore = GetColor(hifore);
    }
    if(hiback != NULL)
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
  gcm = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|GCForeground|
    GCBackground;
  gcv.foreground = fl->HiRelief.fore;
  gcv.background = fl->HiRelief.back;
  gcv.fill_style = FillSolid;
  gcv.plane_mask = AllPlanes;
  gcv.function = GXcopy;
  gcv.graphics_exposures = False;
  gcv.line_width = 0;
  if(fl->HiReliefGC != NULL)
  {
    XFreeGC(dpy,fl->HiReliefGC);
  }
  fl->HiReliefGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcv.foreground = fl->HiRelief.back;
  gcv.background = fl->HiRelief.fore;
  if(fl->HiShadowGC != NULL)
  {
    XFreeGC(dpy,fl->HiShadowGC);
  }
  fl->HiShadowGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  if((Scr.flags & WindowsCaptured)&&(Scr.Hilite != NULL))
  {
    hilight = Scr.Hilite;
    SetBorder(Scr.Hilite,False,True,True,None);
    SetBorder(hilight,True,True,True,None);
  }
}


void SafeDefineCursor(Window w, Cursor cursor)
{
  if (w) XDefineCursor(dpy,w,cursor);
}

void CursorStyle(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                 unsigned long context, char *action,int* Module)
{
  char *cname=NULL, *newcursor=NULL;
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
  nc = atoi(newcursor);
  free(cname);
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

  /* redefine all the windows using cursors */
  fw = Scr.FvwmRoot.next;
  while(fw != NULL)
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
  while(mr != NULL)
  {
    SafeDefineCursor(mr->w,Scr.FvwmCursors[MENU]);
    mr = mr->next;
  }
}

MenuStyle *FindMenuStyle(char *name)
{
  MenuStyle *ms = Scr.menus.DefaultStyle;

  while(ms != NULL )
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
  while(mr != NULL)
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

void DestroyMenuStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                  unsigned long context, char *action,int* Module)
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
  for (mr = Scr.menus.all; mr != NULL; mr = mr->next)
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

  if (ms->look.pStdFont != NULL && ms->look.pStdFont != &Scr.StdFont)
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
  gcm = GCFunction|GCPlaneMask|GCFont|GCGraphicsExposures|
    GCLineWidth|GCForeground|GCBackground;
  gcv.fill_style = FillSolid;
  gcv.font = ms->look.pStdFont->font->fid;
  gcv.plane_mask = AllPlanes;
  gcv.function = GXcopy;
  gcv.graphics_exposures = False;
  gcv.line_width = 0;

  gcv.foreground = ms->look.MenuRelief.fore;
  gcv.background = ms->look.MenuRelief.back;
  if(ms->look.MenuReliefGC != NULL)
    XFreeGC(dpy,ms->look.MenuReliefGC);
  ms->look.MenuReliefGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcv.foreground = ms->look.MenuRelief.back;
  gcv.background = ms->look.MenuRelief.fore;
  if(ms->look.MenuShadowGC != NULL)
    XFreeGC(dpy,ms->look.MenuShadowGC);
  ms->look.MenuShadowGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcv.foreground = (ms->look.f.hasActiveBack) ?
    ms->look.MenuActiveColors.back : ms->look.MenuRelief.back;
  gcv.background = (ms->look.f.hasActiveFore) ?
    ms->look.MenuActiveColors.fore : ms->look.MenuRelief.fore;
  if(ms->look.MenuActiveBackGC != NULL)
    XFreeGC(dpy,ms->look.MenuActiveBackGC);
  ms->look.MenuActiveBackGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcv.foreground = ms->look.MenuColors.fore;
  gcv.background = ms->look.MenuColors.back;
  if(ms->look.MenuGC != NULL)
    XFreeGC(dpy,ms->look.MenuGC);
  ms->look.MenuGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcv.foreground = ms->look.MenuActiveColors.fore;
  gcv.background = ms->look.MenuActiveColors.back;
  if(ms->look.MenuActiveGC != NULL)
    XFreeGC(dpy,ms->look.MenuActiveGC);
  ms->look.MenuActiveGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  if(ms->look.MenuStippleGC != NULL)
    XFreeGC(dpy,ms->look.MenuStippleGC);
  if(Scr.d_depth < 2)
  {
    gcv.fill_style = FillStippled;
    gcv.stipple = Scr.gray_bitmap;
    gcm=GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|
      GCForeground|GCBackground|GCFont|GCStipple|GCFillStyle;
    ms->look.MenuStippleGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

    gcm=GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|
      GCForeground|GCBackground|GCFont;
    gcv.fill_style = FillSolid;
  }
  else
  {
    gcv.foreground = ms->look.MenuStippleColors.fore;
    gcv.background = ms->look.MenuStippleColors.back;
    ms->look.MenuStippleGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);
  }
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
    NULL
  };
  return GetTokenIndex(option, optlist, 0, NULL);
}

static void NewMenuStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action,int* Module)
{
  char *name;
  char *option = NULL;
  char *optstring = NULL;
  char *nextarg;
  char *args;
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
  if (ms != NULL)
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
	if (arg1 != NULL && (xfs = GetFontOrFixed(dpy, arg1)) == NULL)
	{
	  fvwm_msg(ERR,"NewMenuStyle",
		   "Couldn't load font '%s' or 'fixed'\n", arg1);
	  break;
	}
	if (tmpms->look.pStdFont && tmpms->look.pStdFont != &Scr.StdFont)
	{
	  if (tmpms->look.pStdFont->font != NULL)
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
	if (arg1 != NULL)
	{
	  tmpms->look.sidePic = CachePicture(dpy, Scr.Root, IconPath,
					     PixmapPath, arg1, Scr.ColorLimit);
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
	if (arg1 != NULL)
	{
	  tmpms->look.sideColor = GetColor(arg1);
	  tmpms->look.f.hasSideColor = 1;
	}
	break;

#if 0
      case 33: /* PositionHints */
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
  else if (ms != NULL)
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
      while(before->next != NULL)
	before = before->next;
      before->next = tmpms;
    }

  MakeMenus();

  return;
}

static void OldMenuStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		  unsigned long context, char *action,int* Module)
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
	      (animated != NULL && StrEquals(animated, "anim")) ?
	        "Animation" : "AnimationOff");
      NewMenuStyle(eventp, w, tmp_win, context, buffer, Module);
      free(buffer);
    }

  if(fore != NULL)
    free(fore);
  if(back != NULL)
    free(back);
  if(stipple != NULL)
    free(stipple);
  if(font != NULL)
    free(font);
  if(style != NULL)
    free(style);
  if(animated != NULL)
    free(animated);
}


void SetMenuStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		  unsigned long context, char *action,int* Module)
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


void ChangeMenuStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                  unsigned long context, char *action,int* Module)
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
  while(menuname != NULL && *menuname)
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
void SetBorderStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context, char *action,int* Module)
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
	    if ('(' != *action) {
		if (!*action) {
		    fvwm_msg(ERR,"SetBorderStyle",
			     "error in %s border specification", parm);
		    free(parm);
		    return;
		}
		free(parm);
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
void AddTitleStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                   unsigned long context, char *action,int *Module)
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

void SetTitleStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                   unsigned long context, char *action,int* Module)
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
	    while(tmp != NULL)
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
		SetupFrame(tmp,x,y,w,h,True);
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
  gcm = GCFunction|GCPlaneMask|GCFont|GCGraphicsExposures|
    GCLineWidth|GCForeground|GCBackground;
  gcv.fill_style = FillSolid;
  gcv.font = Scr.StdFont.font->fid;
  gcv.plane_mask = AllPlanes;
  gcv.function = GXcopy;
  gcv.graphics_exposures = False;
  gcv.line_width = 0;

  gcv.foreground = Scr.StdColors.fore;
  gcv.background = Scr.StdColors.back;
  if(Scr.StdGC != NULL)
    XFreeGC(dpy, Scr.StdGC);
  Scr.StdGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcv.foreground = Scr.StdRelief.fore;
  gcv.background = Scr.StdRelief.back;
  if(Scr.StdReliefGC != NULL)
    XFreeGC(dpy, Scr.StdReliefGC);
  Scr.StdReliefGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcv.foreground = Scr.StdRelief.back;
  gcv.background = Scr.StdRelief.fore;
  if(Scr.StdShadowGC != NULL)
    XFreeGC(dpy, Scr.StdShadowGC);
  Scr.StdShadowGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

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

  if (Scr.hasIconFont == False)
  {
    Scr.IconFont.font = Scr.StdFont.font;
    ApplyIconFont();
  }

  if (Scr.hasWindowFont == False)
  {
    Scr.DefaultDecor.WindowFont.font = Scr.StdFont.font;
    ApplyWindowFont(&Scr.DefaultDecor);
  }

  for (ms = Scr.menus.DefaultStyle; ms != NULL; ms = ms->next)
  {
    if (ms->look.pStdFont == &Scr.StdFont)
      ms->look.EntryHeight = Scr.StdFont.height + HEIGHT_EXTRA;
    UpdateMenuStyle(ms);
  }
  MakeMenus();
}

void SetDefaultColors(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context, char *action,int* Module)
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

void LoadDefaultFont(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action,int* Module)
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
  if (Scr.StdFont.font != NULL)
    XFreeFont(dpy, Scr.StdFont.font);
  Scr.StdFont.font = xfs;

  ApplyDefaultFontAndColors();
}

void ApplyIconFont(void)
{
  FvwmWindow *tmp;

  Scr.IconFont.height = Scr.IconFont.font->ascent+Scr.IconFont.font->descent;
  Scr.IconFont.y = Scr.IconFont.font->ascent;

  tmp = Scr.FvwmRoot.next;
  while(tmp != NULL)
  {
    RedoIconName(tmp);

    if(tmp->flags& ICONIFIED)
    {
      DrawIconWindow(tmp);
    }
    tmp = tmp->next;
  }
}

void LoadIconFont(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                  unsigned long context, char *action,int* Module)
{
  char *font;
  XFontStruct *newfont;

  action = GetNextToken(action,&font);
  if (!font)
  {
    if (Scr.hasIconFont == True)
    {
      /* reset to default font */
      XFreeFont(dpy, Scr.IconFont.font);
      Scr.hasIconFont = False;
    }
    Scr.IconFont.font = Scr.StdFont.font;
    ApplyIconFont();
    return;
  }

  if ((newfont = GetFontOrFixed(dpy, font))!=NULL)
  {
    if (Scr.IconFont.font != NULL && Scr.hasIconFont == True)
      XFreeFont(dpy, Scr.IconFont.font);
    Scr.hasIconFont = True;
    Scr.IconFont.font = newfont;
    ApplyIconFont();
  }
  else
  {
    fvwm_msg(ERR,"LoadIconFont","Couldn't load font '%s' or 'fixed'\n", font);
  }
  free(font);
}

void ApplyWindowFont(FvwmDecor *fl)
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
  while(tmp != NULL)
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
    SetupFrame(tmp,x,y,w,h,True);
    SetTitleBar(tmp,True,True);
    SetTitleBar(tmp,False,True);
    tmp = tmp->next;
  }
  SetTitleBar(hi,True,True);
}

void LoadWindowFont(XEvent *eventp,Window win,FvwmWindow *tmp_win,
                    unsigned long context, char *action,int* Module)
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
    if (Scr.hasWindowFont)
    {
      XFreeFont(dpy, Scr.DefaultDecor.WindowFont.font);
      Scr.hasWindowFont = False;
      fl->WindowFont.font = Scr.StdFont.font;
      ApplyWindowFont(&Scr.DefaultDecor);
    }
    return;
  }

  if ((newfont = GetFontOrFixed(dpy, font))!=NULL)
  {
    if (fl->WindowFont.font != NULL &&
	(fl != &Scr.DefaultDecor || Scr.hasWindowFont == True))
      XFreeFont(dpy, fl->WindowFont.font);
    if (fl == &Scr.DefaultDecor)
      Scr.hasWindowFont = True;
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

	   XFreeColors(dpy, Scr.FvwmRoot.attr.colormap,
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
	    bf->u.p = CachePicture(dpy, Scr.Root,
				   IconPath,
				   PixmapPath,
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
     *            XFreeColors(dpy, Scr.FvwmRoot.attr.colormap,
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
      mf->u.p = CachePicture(dpy, Scr.Root, IconPath,
			     PixmapPath,
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
void ChangeDecor(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int *Module)
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
    SetupFrame(tmp_win,x,y,width,height,True);
    SetBorder(tmp_win,Scr.Hilite == tmp_win,True,True,None);
}

/*****************************************************************************
 *
 * Destroys an FvwmDecor (veliaa@rpi.edu)
 *
 ****************************************************************************/
void DestroyDecor(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
		 unsigned long context, char *action,int* Module)
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
	while(fw != NULL)
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
void add_item_to_decor(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
		      unsigned long context, char *action,int* Module)
{
    FvwmDecor *fl, *found = NULL;
    char *item = NULL, *s = action;

    last_menu = NULL;

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
	last_decor = found;
    }
}
#endif /* USEDECOR */


/*****************************************************************************
 *
 * Updates window decoration styles (veliaa@rpi.edu)
 *
 ****************************************************************************/
void UpdateDecor(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
		 unsigned long context, char *action,int* Module)
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

    for (; fw != NULL; fw = fw->next)
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
#define SetButtonFlag(a) \
        do { \
             int i; \
             if (multi) { \
               if (multi&1) \
                 for (i=0;i<5;++i) \
                   if (set) \
                     fl->left_buttons[i].flags |= (a); \
                   else \
                     fl->left_buttons[i].flags &= ~(a); \
               if (multi&2) \
                 for (i=0;i<5;++i) \
                   if (set) \
                     fl->right_buttons[i].flags |= (a); \
                   else \
                     fl->right_buttons[i].flags &= ~(a); \
             } else \
                 if (set) \
                   tb->flags |= (a); \
                 else \
                   tb->flags &= ~(a); } while (0)

void ButtonStyle(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                 unsigned long context, char *action,int* Module)
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
void AddButtonStyle(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
		    unsigned long context, char *action,int* Module)
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
		    text = ReadTitleButton(prev, &fl->left_buttons[i], True, i*2+1);
	    if (multi&2)
		for (i=0;i<5;++i)
		    text = ReadTitleButton(prev, &fl->right_buttons[i], True, i*2);
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


void SetEnv(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
	    unsigned long context, char *action,int* Module)
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
    else if(StrEquals(condition,"Raised"))
      mask->onFlags |= RAISED;
    else if(StrEquals(condition,"!Raised"))
      mask->offFlags |= RAISED;
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

  if(prev_condition != NULL)
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

  if(Scr.Focus != NULL)
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
    while((fw != NULL)&&(found==NULL)&&(fw != &Scr.FvwmRoot))
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
        while((fw) && (fw->next != NULL))
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

void PrevFunc(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
              unsigned long context, char *action,int* Module)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, -1, &restofline);
  if(found != NULL && restofline != NULL)
  {
    ExecuteFunction(restofline,found,eventp,C_WINDOW,*Module);
  }

}

void NextFunc(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
              unsigned long context, char *action,int* Module)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, 1, &restofline);
  if(found != NULL && restofline != NULL)
  {
    ExecuteFunction(restofline,found,eventp,C_WINDOW,*Module);
  }

}

void NoneFunc(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
              unsigned long context, char *action,int* Module)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, 1, &restofline);
  if(found == NULL && restofline != NULL)
  {
    ExecuteFunction(restofline,NULL,eventp,C_ROOT,*Module);
  }
}

void CurrentFunc(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
              unsigned long context, char *action,int* Module)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, 0, &restofline);
  if(found != NULL && restofline != NULL)
  {
    ExecuteFunction(restofline,found,eventp,C_WINDOW,*Module);
  }
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
void DirectionFunc(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
		   unsigned long context, char *action, int *Module)
{
  char *directions[] = { "North", "East", "South", "West", "NorthEast",
			 "SouthEast", "SouthWest", "NorthWest", NULL };
  int my_x;
  int my_y;
  int his_x;
  int his_y;
  int score;
  int offset;
  int distance;
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
  if (tmp_win != NULL)
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
  for (window = Scr.FvwmRoot.next; window != NULL; window = window->next) {
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

  if (best_window != NULL)
    ExecuteFunction(restofline, best_window, eventp, C_WINDOW, *Module);

  FreeConditionMask(&mask);
}

void WindowIdFunc(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                  unsigned long context, char *action,int* Module)
{
  FvwmWindow *found=NULL,*t;
  char *num;
  unsigned long win;

  action = GetNextToken(action, &num);

  if (num)
    {
      win = (unsigned long)strtol(num,NULL,0); /* SunOS doesn't have strtoul */
      free(num);
    }
  else
    win = 0;
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    if (t->w == win)
    {
      found = t;
      break;
    }
  }
  if(found)
  {
    ExecuteFunction(action,found,eventp,C_WINDOW,*Module);
  }
}


void module_zapper(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                   unsigned long context, char *action,int* Module)
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
void Recapture(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
               unsigned long context, char *action,int* Module)
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

void SetGlobalOptions(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                      unsigned long context, char *action,int* Module)
{
  char *opt;

  /* fvwm_msg(DBG,"SetGlobalOptions","init action == '%s'\n",action); */
  for (action = GetNextOption(action, &opt); opt;
       action = GetNextOption(action, &opt))
  {
    /* fvwm_msg(DBG,"SetGlobalOptions"," opt == '%s'\n",opt); */
    /* fvwm_msg(DBG,"SetGlobalOptions"," remaining == '%s'\n",
       action?action:"(NULL)"); */
    if (StrEquals(opt,"SMARTPLACEMENTISREALLYSMART"))
    {
      Scr.SmartPlacementIsClever = True;
    }
    else if (StrEquals(opt,"SMARTPLACEMENTISNORMAL"))
    {
      Scr.SmartPlacementIsClever = False;
    }
    else if (StrEquals(opt,"CLICKTOFOCUSDOESNTPASSCLICK"))
    {
      Scr.ClickToFocusPassesClick = False;
    }
    else if (StrEquals(opt,"CLICKTOFOCUSPASSESCLICK"))
    {
      Scr.ClickToFocusPassesClick = True;
    }
    else if (StrEquals(opt,"CLICKTOFOCUSDOESNTRAISE"))
    {
      Scr.ClickToFocusRaises = False;
    }
    else if (StrEquals(opt,"CLICKTOFOCUSRAISES"))
    {
      Scr.ClickToFocusRaises = True;
    }
    else if (StrEquals(opt,"MOUSEFOCUSCLICKDOESNTRAISE"))
    {
      Scr.MouseFocusClickRaises = False;
    }
    else if (StrEquals(opt,"MOUSEFOCUSCLICKRAISES"))
    {
      Scr.MouseFocusClickRaises = True;
    }
    else if (StrEquals(opt,"NOSTIPLEDTITLES"))
    {
      Scr.StipledTitles = False;
    }
    else if (StrEquals(opt,"STIPLEDTITLES"))
    {
      Scr.StipledTitles = True;
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

void Emulate(XEvent *eventp, Window junk, FvwmWindow *tmp_win,
	     unsigned long context, char *action, int* Module)
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

void SetColorLimit(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                   unsigned long context, char *action,int* Module)
{
  int val;

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
void set_animation(XEvent *eventp,Window w,FvwmWindow *tmp_win,
			  unsigned long context, char *action,int* Module)
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
