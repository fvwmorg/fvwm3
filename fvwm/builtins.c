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

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

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

  if((*context != C_ROOT)&&(*context != C_NO_CONTEXT))
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
      Keyboard_shortcuts(eventp,FinishEvent);
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
void FocusOn(FvwmWindow *t,int DeIconifyOnly)
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
  SetFocus(t->w,t,0);
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
    ConstrainSize (tmp_win, &new_width, &new_height);
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

  mr = Scr.AllMenus;
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
    token = "";
  mr = FollowMenuContinuations(FindPopup(token),&mrPrior);
  if(mr == NULL)
    mr = NewMenuRoot(token, 0);
  last_menu = mr;

  rest = GetNextToken(rest,&item);
  AddToMenu(mr, item,rest,TRUE /* pixmap scan */, TRUE);
  if (item)
    free(item);
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
  mr = FindPopup(token);
  if (token)
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
  mr = FindPopup(token);
  if(mr == NULL)
    mr = NewMenuRoot(token, 1);
  last_menu = mr;
  if (token)
    free(token);
  rest = GetNextToken(rest,&item);
  AddToMenu(mr, item,rest,FALSE,FALSE);
  if (item)
    free(item);

  return;
}


void Nop_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,unsigned long context,
              char *action, int *Module)
{

}



void movecursor(XEvent *eventp,Window w,FvwmWindow *tmp_win,unsigned long context,
		char *action, int *Module)
{
#ifndef NON_VIRTUAL
  int x,y,delta_x,delta_y,warp_x,warp_y;
  int val1, val2, val1_unit,val2_unit,n;

  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
  if (n != 2)
  {
    fvwm_msg(ERR, "movecursor", "CursorMove needs 2 arguments");
    return;
  }

  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
                 &x,&y,&JunkX, &JunkY, &JunkMask);
  delta_x = 0;
  delta_y = 0;
  warp_x = 0;
  warp_y = 0;
  if(x >= Scr.MyDisplayWidth -2)
  {
    delta_x = Scr.EdgeScrollX;
    warp_x = Scr.EdgeScrollX - 4;
  }
  if(y>= Scr.MyDisplayHeight -2)
  {
    delta_y = Scr.EdgeScrollY;
    warp_y = Scr.EdgeScrollY - 4;
  }
  if(x < 2)
  {
    delta_x = -Scr.EdgeScrollX;
    warp_x =  -Scr.EdgeScrollX + 4;
  }
  if(y < 2)
  {
    delta_y = -Scr.EdgeScrollY;
    warp_y =  -Scr.EdgeScrollY + 4;
  }
  if(Scr.Vx + delta_x < 0)
    delta_x = -Scr.Vx;
  if(Scr.Vy + delta_y < 0)
    delta_y = -Scr.Vy;
  if(Scr.Vx + delta_x > Scr.VxMax)
    delta_x = Scr.VxMax - Scr.Vx;
  if(Scr.Vy + delta_y > Scr.VyMax)
    delta_y = Scr.VyMax - Scr.Vy;
  if((delta_x!=0)||(delta_y!=0))
  {
    MoveViewport(Scr.Vx + delta_x,Scr.Vy+delta_y,True);
    XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
                 Scr.MyDisplayHeight,
                 x - warp_x,
                 y - warp_y);
  }
#endif
  XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
	       Scr.MyDisplayHeight, x + val1*val1_unit/100-warp_x,
	       y+val2*val2_unit/100 - warp_y);
}


void iconify_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,char *action, int *Module)

{
  int val1;
  int val1_unit,n;

  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,
		     ButtonRelease))
    return;

  n = GetOneArgument(action, &val1, &val1_unit);

  if (tmp_win->flags & ICONIFIED)
  {
    if(val1 <=0)
      DeIconify(tmp_win);
  }
  else
  {
    if(check_allowed_function2(F_ICONIFY,tmp_win) == 0)
    {
      XBell(dpy, 0);
      return;
    }
    if(val1 >=0)
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
  if (0 && strncasecmp(action,"exec",4)!=0)
  {
    cmd = (char *)safemalloc(strlen(action)+6);
    strcpy(cmd,"exec ");
    strcat(cmd,action);
  }
  else
  {
    cmd = strdup(action);
  }
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
    }
  }
}


void flip_focus_reorder(FvwmWindow *w)
{
  if( w->next ) w->next->prev = w->prev;
  if( w->prev ) w->prev->next = w->next;
  w->next = Scr.FvwmRoot.next;
  w->prev = &Scr.FvwmRoot;
  if(Scr.FvwmRoot.next)Scr.FvwmRoot.next->prev = w;
  Scr.FvwmRoot.next = w;
}

void flip_focus_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action, int *Module)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;

  /* Reorder the window list */
  if( Scr.Focus ){
    flip_focus_reorder(Scr.Focus);
  }
  if( tmp_win != Scr.Focus ){
    flip_focus_reorder(tmp_win);
  }

  FocusOn(tmp_win,0);
}


void focus_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action, int *Module)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;

  FocusOn(tmp_win,0);
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

/* the function for the "Popup" command */
void popup_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int *Module)
{
  MenuRoot *menu;
  extern int menuFromFrameOrWindowOrTitlebar;
  MenuItem *miExecuteAction;
  char *menu_name = NULL;
  MenuOptions mops;

  mops.flags = 0;
  action = GetNextToken(action,&menu_name);
  GetMenuOptions(action,w,tmp_win,NULL,&mops);
  menu = FindPopup(menu_name);
  if(menu == NULL)
  {
    if(menu_name != NULL)
    {
      fvwm_msg(ERR,"popup_func","No such menu %s",menu_name);
      free(menu_name);
    }
    return;
  }

  if(menu_name != NULL)
    free(menu_name);

  menuFromFrameOrWindowOrTitlebar = FALSE;
  do_menu(menu, NULL, &miExecuteAction, 0, FALSE,
	  (eventp->type == KeyPress) ? (XEvent *)1 : NULL, &mops);
}

/* the function for the "Menu" command */
void staysup_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                  unsigned long context, char *action,int *Module)
{
  MenuRoot *menu;
  extern int menuFromFrameOrWindowOrTitlebar;
  char *default_action = NULL, *menu_name = NULL;
  MenuStatus menu_retval;
  MenuItem *miExecuteAction = NULL;
  MenuOptions mops;
  XEvent *teventp;

  mops.flags = 0;
  action = GetNextToken(action,&menu_name);
  action = GetMenuOptions(action,w,tmp_win,NULL,&mops);
  GetNextToken(action,&default_action);
  menu = FindPopup(menu_name);
  if(menu == NULL)
  {
    if(menu_name != NULL)
    {
      fvwm_msg(ERR,"staysup_func","No such menu %s",menu_name);
      free(menu_name);
    }
    if(default_action != NULL)
      free(default_action);
    return;
  }
  if(menu_name != NULL)
    free(menu_name);
  menuFromFrameOrWindowOrTitlebar = FALSE;

  if (!default_action && eventp && eventp->type == KeyPress)
    teventp = (XEvent *)1;
  else
    teventp = eventp;
  menu_retval = do_menu(menu, NULL, &miExecuteAction, 0, TRUE, teventp, &mops);

  if (menu_retval == MENU_DOUBLE_CLICKED && default_action && *default_action)
    ExecuteFunction(default_action,tmp_win,eventp,context,*Module);
  if(default_action != NULL)
    free(default_action);
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
  if (action && *action)
  {
    int len=strlen(action);
    if (action[len-1]=='\n')
      action[len-1]='\0';
    fvwm_msg(INFO,"Echo",action);
  }
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
  ** if edgescroll >1000 and < 100000m
  ** wrap at edges of desktop (a "spherical" desktop)
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
  int val1, val2, val1_unit,val2_unit,n;

  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
  if(n != 2)
  {
    fvwm_msg(ERR,"SetEdgeResistance","EdgeResistance requires two arguments");
    return;
  }

  Scr.ScrollResistance = val1;
  Scr.MoveResistance = val2;
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
  int val1;
  int val1_unit,n;

  n = GetOneArgument(action, &val1, &val1_unit);
  if(n != 1)
  {
    fvwm_msg(ERR,"SetClick","ClickTime requires 1 argument");
    return;
  }

  if (val1 < 0)
    Scr.ClickTime = 0;
  else
    Scr.ClickTime = val1;
}

void SetXOR(XEvent *eventp,Window w,FvwmWindow *tmp_win,
            unsigned long context, char *action,int* Module)
{
  int val1;
  int val1_unit,n;
  XGCValues gcv;
  unsigned long gcm;

  n = GetOneArgument(action, &val1, &val1_unit);
  if(n != 1)
  {
    fvwm_msg(ERR,"SetXOR","XORValue requires 1 argument");
    return;
  }

  gcm = GCFunction|GCLineWidth|GCForeground|GCSubwindowMode;
  gcv.function = GXxor;
  gcv.line_width = 0;
  /* use passed in value, or try to calculate appropriate value if 0 */
  /* ctwm method: */
  /* gcv.foreground = (val1)?(val1):((((unsigned long) 1) << Scr.d_depth) - 1); */
  /* Xlib programming manual suggestion: */
  gcv.foreground = (val1)?(val1):(BlackPixel(dpy,Scr.screen) ^ WhitePixel(dpy,Scr.screen));
  gcv.subwindow_mode = IncludeInferiors;
  Scr.DrawGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);
}

void SetOpaque(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	       unsigned long context, char *action,int* Module)
{
  int val1;
  int val1_unit,n;

  n = GetOneArgument(action, &val1, &val1_unit);
  if(n != 1)
  {
    fvwm_msg(ERR,"SetOpaque","OpaqueMoveSize requires 1 argument");
    return;
  }

  Scr.OpaqueSize = val1;
}


void SetDeskSize(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                 unsigned long context, char *action,int* Module)
{
  int val1, val2, val1_unit,val2_unit,n;

  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
  if(n != 2)
  {
    fvwm_msg(ERR,"SetDeskSize","DesktopSize requires two arguments");
    return;
  }
  if((val1_unit != Scr.MyDisplayWidth)||
     (val2_unit != Scr.MyDisplayHeight))
  {
    fvwm_msg(ERR,"SetDeskSize","DeskTopSize arguments should be unitless");
  }

  Scr.VxMax = val1*Scr.MyDisplayWidth - Scr.MyDisplayWidth;
  Scr.VyMax = val2*Scr.MyDisplayHeight - Scr.MyDisplayHeight;
  if(Scr.VxMax <0)
    Scr.VxMax = 0;
  if(Scr.VyMax <0)
    Scr.VyMax = 0;
  BroadcastPacket(M_NEW_PAGE, 5,
                  Scr.Vx, Scr.Vy, Scr.CurrentDesk, Scr.VxMax, Scr.VyMax);

  checkPanFrames();
}

#ifdef XPM
char *PixmapPath = FVWM_ICONDIR;
void setPixmapPath(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                   unsigned long context, char *action,int* Module)
{
  static char *ptemp = NULL;
  char *tmp;

  if(ptemp == NULL)
    ptemp = PixmapPath;

  if((PixmapPath != ptemp)&&(PixmapPath != NULL))
    free(PixmapPath);
  tmp = stripcpy(action);
  PixmapPath = envDupExpand(tmp, 0);
  free(tmp);
}
#endif

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
  extern char *white,*black;
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
    fl->HiColors.back  = GetColor(white);
    fl->HiColors.fore  = GetColor(black);
    fl->HiRelief.back  = GetColor(black);
    fl->HiRelief.fore  = GetColor(white);
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
  free(cname);
  nc = atoi(newcursor);
  free(newcursor);
  if ((nc < 0) || (nc >= XC_num_glyphs) || ((nc % 2) != 0))
  {
    fvwm_msg(ERR,"CursorStyle","Bad cursor number %s",newcursor);
    return;
  }

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
  mr = Scr.AllMenus;
  while(mr != NULL)
  {
    SafeDefineCursor(mr->w,Scr.FvwmCursors[MENU]);
    mr = mr->next;
  }
}

void SetMenuStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                  unsigned long context, char *action,int* Module)
{
#if 0 /* new parse syntax -- unfinished */
  if (action && *action)
  {
    line = action;
    while (line && *line)
    {
      line = GetNextToken(line, &tok);
      if (StrEquals(tok,"Color"))
      {
        /* Color fg/bg/st */
      }
      else if (StrEquals(tok,"mwm"))
      {
        Scr.flags |= MWMMenus;
      }
      else if (StrEquals(tok,"fvwm"))
      {
        Scr.flags &= ~MWMMenus;
      }
      else if (StrEquals(tok,"Font"))
      {
      }
      else if (StrEquals(tok,"NoWarp"))
      {
      }
      else if (StrEquals(tok,"Warp"))
      {
      }
      else if (StrEquals(tok,"Warp"))
      {
      }
      if (tok)
        free(tok);
    }
  }
#else
  XGCValues gcv;
  unsigned long gcm;
  char *fore=NULL, *back=NULL, *stipple = NULL, *font= NULL, *style = 0;
  char *animated=NULL;
  extern char *white,*black;
  int wid,hei;

  action = GetNextToken(action,&fore);
  action = GetNextToken(action,&back);
  action = GetNextToken(action,&stipple);
  action = GetNextToken(action,&font);
  action = GetNextToken(action,&style);
  action = GetNextToken(action,&animated);

  if((style != NULL)&&(strncasecmp(style,"MWM",3)==0)) {
    Scr.menu_type = MWM;
  } else if((style != NULL)&&(strncasecmp(style,"WIN",3)==0)) {
    Scr.menu_type = WIN;
  } else {
    Scr.menu_type = FVWM;
  }

  if ((animated != NULL) && (strncasecmp(animated,"anim",4)==0)) {
    Scr.flags |= AnimatedMenus;
  }
  else {
    Scr.flags &= ~AnimatedMenus;
  }

  if(Scr.d_depth > 2)
  {
    if(fore != NULL)
    {
      Scr.MenuColors.fore = GetColor(fore);
    }
    if(back != NULL)
    {
      Scr.MenuColors.back = GetColor(back);
    }
    Scr.MenuRelief.back  = GetShadow(Scr.MenuColors.back);
    Scr.MenuRelief.fore  = GetHilite(Scr.MenuColors.back);
    Scr.MenuStippleColors.back = Scr.MenuColors.back;
    Scr.MenuStippleColors.fore = GetColor(stipple);
  }
  else
  {
    Scr.MenuColors.back  = GetColor(white);
    Scr.MenuColors.fore  = GetColor(black);
    Scr.MenuRelief.back  = GetColor(black);
    Scr.MenuRelief.fore  = GetColor(white);
    Scr.MenuStippleColors.back = GetColor(white);
    Scr.MenuStippleColors.fore = GetColor(black);
  }

  if (Scr.StdFont.font != NULL)
    XFreeFont(dpy, Scr.StdFont.font);
  if ((font == NULL)||
      (Scr.StdFont.font = GetFontOrFixed(dpy, font)) == NULL)
  {
    fvwm_msg(ERR,"SetMenuStyle","Couldn't load font '%s' or 'fixed'\n",
             (font==NULL)?("NULL"):(font));
    exit(1);
  }
  Scr.StdFont.height = Scr.StdFont.font->ascent + Scr.StdFont.font->descent;
  Scr.StdFont.y = Scr.StdFont.font->ascent;
  Scr.EntryHeight = Scr.StdFont.height + HEIGHT_EXTRA;

  gcm = GCFunction|GCPlaneMask|GCFont|GCGraphicsExposures|
    GCLineWidth|GCForeground|GCBackground;
  gcv.foreground = Scr.MenuRelief.fore;
  gcv.background = Scr.MenuRelief.back;
  gcv.fill_style = FillSolid;
  gcv.font = Scr.StdFont.font->fid;
  gcv.plane_mask = AllPlanes;
  gcv.function = GXcopy;
  gcv.graphics_exposures = False;
  gcv.line_width = 0;
  if(Scr.MenuReliefGC != NULL)
  {
    XFreeGC(dpy,Scr.MenuReliefGC);
  }
  Scr.MenuReliefGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcv.foreground = Scr.MenuRelief.back;
  gcv.background = Scr.MenuRelief.fore;
  if(Scr.MenuShadowGC != NULL)
  {
    XFreeGC(dpy,Scr.MenuShadowGC);
  }
  Scr.MenuShadowGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);
  gcv.foreground = Scr.MenuColors.fore;
  gcv.background = Scr.MenuColors.back;
  if(Scr.MenuGC != NULL)
  {
    XFreeGC(dpy,Scr.MenuGC);
  }
  Scr.MenuGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);
  if(Scr.d_depth < 2)
  {
    gcv.fill_style = FillStippled;
    gcv.stipple = Scr.gray_bitmap;
    gcm=GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|GCForeground|
      GCBackground|GCFont|GCStipple|GCFillStyle;
    Scr.MenuStippleGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

    gcm=GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|GCForeground|
      GCBackground|GCFont;
    gcv.fill_style = FillSolid;
  }
  else
  {
    gcv.foreground = Scr.MenuStippleColors.fore;
    gcv.background = Scr.MenuStippleColors.back;
    Scr.MenuStippleGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);
  }
  if(Scr.SizeWindow != None)
  {
    Scr.SizeStringWidth = XTextWidth (Scr.StdFont.font,
                                      " +8888 x +8888 ", 15);
    wid = Scr.SizeStringWidth+SIZE_HINDENT*2;
    hei = Scr.StdFont.height+SIZE_VINDENT*2;
    if(USING_MWM_MENUS)
    {
      XMoveResizeWindow(dpy,Scr.SizeWindow,
                        Scr.MyDisplayWidth/2 -wid/2,
                        Scr.MyDisplayHeight/2 - hei/2,
                        wid,hei);
    }
    else
    {
      XMoveResizeWindow(dpy,Scr.SizeWindow,0, 0, wid,hei);
    }
  }


  if(Scr.SizeWindow != None)
  {
    XSetWindowBackground(dpy,Scr.SizeWindow,Scr.MenuColors.back);
  }
  MakeMenus();
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
#endif /* 0 */
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
                   unsigned long context, char *action,int* Module)
{
#ifdef USEDECOR
    FvwmDecor *fl = cur_decor ? cur_decor : &Scr.DefaultDecor;
#else
    FvwmDecor *fl = &Scr.DefaultDecor;
#endif
    char *parm=NULL, *prev;
    prev = action;
    action = GetNextToken(action,&parm);
    while(parm)
    {
	if (!(action = ReadTitleButton(prev, &fl->titlebar, True, -1))) {
	    free(parm);
	    break;
	}
	prev = action;
	action = GetNextToken(action,&parm);
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

void LoadIconFont(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                  unsigned long context, char *action,int* Module)
{
  char *font;
  FvwmWindow *tmp;

  action = GetNextToken(action,&font);
  if (!font)
    return ;

  if (Scr.IconFont.font != NULL)
    XFreeFont(dpy, Scr.IconFont.font);
  if ((Scr.IconFont.font = GetFontOrFixed(dpy, font))==NULL)
  {
    fvwm_msg(ERR,"LoadIconFont","Couldn't load font '%s' or 'fixed'\n", font);
    free(font);
    return;
  }
  Scr.IconFont.height=
    Scr.IconFont.font->ascent+Scr.IconFont.font->descent;
  Scr.IconFont.y = Scr.IconFont.font->ascent;

  free(font);
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

void LoadWindowFont(XEvent *eventp,Window win,FvwmWindow *tmp_win,
                    unsigned long context, char *action,int* Module)
{
  char *font;
  FvwmWindow *tmp,*hi;
  int x,y,w,h,extra_height;
  XFontStruct *newfont;
#ifdef USEDECOR
  FvwmDecor *fl = cur_decor ? cur_decor : &Scr.DefaultDecor;
#else
  FvwmDecor *fl = &Scr.DefaultDecor;
#endif

  action = GetNextToken(action,&font);
  if (!font)
    return;

  if ((newfont = GetFontOrFixed(dpy, font))!=NULL)
  {
    if (fl->WindowFont.font != NULL)
      XFreeFont(dpy, fl->WindowFont.font);
    fl->WindowFont.font = newfont;
    fl->WindowFont.height=
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
  {
    fvwm_msg(ERR,"LoadWindowFont","Couldn't load font '%s' or 'fixed'\n",
            font);
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
	if(verbose)fvwm_msg(ERR, "ReadButtonFace", "error in face: %s", action);
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
		num = sscanf(s,"%dx%d@%d%n",&vc->x[i],&vc->y[i],&vc->line_style[i],
			     &offset);
		if(num != 3)
		{
		    if(verbose)fvwm_msg(ERR,"ReadButtonFace",
					"Bad button style (3) in line %s",action);
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
	    npixels = atoi(item); free(item);

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
		nsegs = atoi(item); free(item);
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
				    "multi gradient lenghts must sum to 100");
		for (i = 0; i <= nsegs; ++i)
		    free(s_colors[i]);
		free(s_colors);
		return False;
	    }

	    if (npixels < 2) npixels = 2;
	    if (npixels > 128) npixels = 128;

	    pixels = AllocNonlinearGradient(s_colors, perc, nsegs, npixels);
	    for (i = 0; i <= nsegs; ++i)
		free(s_colors[i]);
	    free(s_colors);

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
#ifdef XPM
				   PixmapPath,
#else
				   NULL,
#endif
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
    FvwmDecor *fl = &Scr.DefaultDecor, *found = NULL;
    char *item = NULL, *s = action;

    last_menu = NULL;

    s = GetNextToken(s, &item);

    if (!s || !item)
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
			if (multi&1)
			    for (i=0;i<5;++i)
				if (set)
				    fl->left_buttons[i].flags = 0;
				else
				    fl->left_buttons[i].flags = ~0;
			if (multi&2)
			    for (i=0;i<5;++i)
				if (set)
				    fl->right_buttons[i].flags = 0;
				else
				    fl->right_buttons[i].flags = ~0;
		    } else
			if (set)
			    tb->flags = 0;
			else
			    tb->flags = ~0;
		}
                else if (strncasecmp(tok,"MWMDecorMenu",12)==0)
                  SetButtonFlag(MWMDecorMenu);
                else if (strncasecmp(tok,"MWMDecorMin",11)==0)
                  SetButtonFlag(MWMDecorMinimize);
                else if (strncasecmp(tok,"MWMDecorMax",11)==0)
                  SetButtonFlag(MWMDecorMaximize);
		else
		    fvwm_msg(ERR,"ButtonStyle",
			     "unknown title button flag %s -- line: %s", tok, text);
		if (set) free(tok);
		else free(tok - 1);
		text = GetNextToken(text, &tok);
	    }
	    free(parm);
	    break;
	}
	else {
	    if (multi) {
		int i;
		if (multi&1)
		    for (i=0;i<5;++i)
			text = ReadTitleButton(prev, &fl->left_buttons[i], False, i*2+1);
		if (multi&2)
		    for (i=0;i<5;++i)
			text = ReadTitleButton(prev, &fl->right_buttons[i], False, i*2);
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
    action = GetNextToken(action,&szValue);
    if (!szVar || !szValue)
      {
	if (szVar)
	  free(szVar);
	if (szValue)
	  free(szValue);
	return;
      }

    szPutenv = safemalloc(strlen(szVar)+strlen(szValue)+2);
    sprintf(szPutenv,"%s=%s",szVar,szValue);
    putenv(szPutenv);
    free(szVar);
    free(szValue);
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
  int l,pass = 0;
  FvwmWindow *fw, *found = NULL;
  char *t,*tstart,*name = NULL, *expression, *condition, *prev_condition=NULL;
  char *orig_expr;
  Bool needsCurrentDesk = 0;
  Bool needsCurrentPage = 0;

  Bool needsName = 0;
  Bool needsNotName = 0;
  Bool useCirculateHit = (Direction) ? 0 : 1; /* override for Current [] */
  Bool useCirculateHitIcon = (Direction) ? 0 : 1;
  unsigned long onFlags = 0;
  unsigned long offFlags = 0;
  char closeopt;

  l=0;

  if(action == NULL)
    return NULL;

  t = action;
  while(isspace(*t)&&(*t!= 0))
    t++;
  if(*t == '[' || *t == '(')
  {
    if (*t == '[')
      closeopt = ']';
    else
      closeopt = ')';
    t++;
    tstart = t;

    while((*t !=0)&&(*t != closeopt))
    {
      t++;
      l++;
    }
    if(*t == 0)
    {
      fvwm_msg(ERR,"Circulate","Conditionals require closing parenthesis");
      return NULL;
    }

    *restofline = t+1;

    orig_expr = expression = safemalloc(l+1);
    strncpy(expression,tstart,l);
    expression[l] = 0;
    expression = GetNextToken(expression,&condition);
    while (condition)
    {
      if     (StrEquals(condition,"Iconic"))
        onFlags |= ICONIFIED;
      else if(StrEquals(condition,"!Iconic"))
        offFlags |= ICONIFIED;
      else if(StrEquals(condition,"Visible"))
        onFlags |= VISIBLE;
      else if(StrEquals(condition,"!Visible"))
        offFlags |= VISIBLE;
      else if(StrEquals(condition,"Sticky"))
        onFlags |= STICKY;
      else if(StrEquals(condition,"!Sticky"))
        offFlags |= STICKY;
      else if(StrEquals(condition,"Maximized"))
        onFlags |= MAXIMIZED;
      else if(StrEquals(condition,"!Maximized"))
        offFlags |= MAXIMIZED;
      else if(StrEquals(condition,"Transient"))
        onFlags |= TRANSIENT;
      else if(StrEquals(condition,"!Transient"))
        offFlags |= TRANSIENT;
      else if(StrEquals(condition,"Raised"))
        onFlags |= RAISED;
      else if(StrEquals(condition,"!Raised"))
        offFlags |= RAISED;
      else if(StrEquals(condition,"CurrentDesk"))
        needsCurrentDesk = 1;
      else if(StrEquals(condition,"CurrentPage"))
      {
        needsCurrentDesk = 1;
        needsCurrentPage = 1;
      }
      else if(StrEquals(condition,"CurrentPageAnyDesk") ||
              StrEquals(condition,"CurrentScreen"))
        needsCurrentPage = 1;
      else if(StrEquals(condition,"CirculateHit"))
        useCirculateHit = 1;
      else if(StrEquals(condition,"CirculateHitIcon"))
        useCirculateHitIcon = 1;
      else if(!needsName && !needsNotName) /* only 1st name to avoid mem leak */
      {
        name = condition;
        condition = NULL;
        if (name[0] == '!')
        {
          needsNotName = 1;
          name++;
        }
        else
          needsName = 1;
      }
      if(prev_condition)free(prev_condition);
      prev_condition = condition;
      expression = GetNextToken(expression,&condition);
    }
    if(prev_condition != NULL)
      free(prev_condition);
    if(orig_expr != NULL)
      free(orig_expr);
  }
  else
    *restofline = t;

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
    fw = Scr.FvwmRoot.prev;

  while((pass < 3)&&(found == NULL))
  {
    while((fw != NULL)&&(found==NULL)&&(fw != &Scr.FvwmRoot))
    {
#ifdef FVWM_DEBUG_MSGS
	fvwm_msg(DBG,"Circulate","Trying %s",fw->name);
#endif /* FVWM_DEBUG_MSGS */
      /* Make CirculateUp and CirculateDown take args. by Y.NOMURA */
      if ((onFlags & fw->flags) == onFlags &&
          (offFlags & fw->flags) == 0 &&
          (useCirculateHit || !(fw->flags & CirculateSkip)) &&
          ((useCirculateHitIcon && fw->flags & ICONIFIED) ||
           !(fw->flags & CirculateSkipIcon && fw->flags & ICONIFIED)) &&
          (!needsCurrentDesk || fw->Desk == Scr.CurrentDesk) &&
          (!needsCurrentPage || (fw->frame_x < Scr.MyDisplayWidth &&
                                 fw->frame_y < Scr.MyDisplayHeight &&
                                 fw->frame_x + fw->frame_width > 0 &&
                                 fw->frame_y + fw->frame_height > 0)
            ) &&
          (!needsName ||
           matchWildcards(name, fw->name) ||
           matchWildcards(name, fw->icon_name) ||
           fw->class.res_class && matchWildcards(name, fw->class.res_class) ||
           fw->class.res_name && matchWildcards(name, fw->class.res_name)
            ) &&
          (!needsNotName ||
           !(matchWildcards(name, fw->name) ||
             matchWildcards(name, fw->icon_name) ||
             fw->class.res_class && matchWildcards(name, fw->class.res_class)||
             fw->class.res_name && matchWildcards(name, fw->class.res_name)
             )
            )
        )
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
        if (needsName)
          free(name);
        else if (needsNotName)
          free(name - 1);
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
  if (needsName)
    free(name);
  else if (needsNotName)
    free(name - 1);
  return found;

}

void PrevFunc(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
              unsigned long context, char *action,int* Module)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, -1, &restofline);
  if(found != NULL)
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
  if(found != NULL)
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
  if(found == NULL)
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
  if(found != NULL)
  {
    ExecuteFunction(restofline,found,eventp,C_WINDOW,*Module);
  }
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
  BlackoutScreen(); /* if they want to hide the recapture */
  CaptureAllWindows();
  UnBlackoutScreen();
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
         /*  RBW - 11/02/1998  */
    else if (StrEquals(opt,"STARTSONPAGEMODIFIESUSPOSITION"))
    {
      Scr.ModifyUSP = True;
    }
    else if (StrEquals(opt,"STARTSONPAGEHONORSUSPOSITION"))
    {
      Scr.ModifyUSP = False;
    }
    else if (StrEquals(opt,"CAPTUREHONORSSTARTSONPAGE"))
    {
      Scr.CaptureHonorsStartsOnPage = True;
    }
    else if (StrEquals(opt,"CAPTUREIGNORESSTARTSONPAGE"))
    {
      Scr.CaptureHonorsStartsOnPage = False;
    }
    else if (StrEquals(opt,"RECAPTUREHONORSSTARTSONPAGE"))
    {
      Scr.RecaptureHonorsStartsOnPage = True;
    }
    else if (StrEquals(opt,"RECAPTUREIGNORESSTARTSONPAGE"))
    {
      Scr.RecaptureHonorsStartsOnPage = False;
    }
    else if (StrEquals(opt,"ACTIVEPLACEMENTHONORSSTARTSONPAGE"))
    {
      Scr.ActivePlacementHonorsStartsOnPage = True;
    }
    else if (StrEquals(opt,"ACTIVEPLACEMENTIGNORESSTARTSONPAGE"))
    {
      Scr.ActivePlacementHonorsStartsOnPage = False;
    }
    else
      fvwm_msg(ERR,"GlobalOpts","Unknown Global Option '%s'",opt);
    if (opt) /* should never be null, but checking anyways... */
      free(opt);
  }
  if (opt)
    free(opt);
}

void SetColorLimit(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                   unsigned long context, char *action,int* Module)
{
  int val1;
  int val1_unit,n;

  n = GetOneArgument(action, &val1, &val1_unit);
  if(n != 1) {
    fvwm_msg(ERR,"SetColorLimit","ColorLimit requires 1 argument, found %d",n);
    return;
  }

  Scr.ColorLimit = (long)val1;
}


extern float rgpctMovementDefault[32];
extern int cpctMovementDefault;
extern int cmsDelayDefault;
extern int c10msDelaysBeforePopup;


/* set animation parameters */
void set_animation(XEvent *eventp,Window w,FvwmWindow *tmp_win,
			  unsigned long context, char *action,int* Module)
{
  char *opt = strtok(action," \t");
  int delay;
  float pct;
  int i = 0;
  if (sscanf(opt,"%d",&delay) != 1) {
    fvwm_msg(ERR,"SetAnimation",
	     "Improper milli-second delay as first argument");
    return;
  }
  if (delay > 500) {
    fvwm_msg(WARN,"SetAnimation",
	     "Using longer than .5 seconds as between frame animation delay");
  }
  cmsDelayDefault = delay;
  while ((opt =strtok(NULL," \t"))) {
    if (sscanf(opt,"%f",&pct) != 1) {
      fvwm_msg(ERR,"SetAnimation",
	       "Use fractional values ending in 1.0 as args 2 and on");
      return;
    }
    rgpctMovementDefault[i++] = pct;
  }
  /* No pct entries means don't change them at all */
  if (i>0 && rgpctMovementDefault[i-1] != 1.0) {
    rgpctMovementDefault[i++] = 1.0;
  }
}

void set_menudelay(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		   unsigned long context, char *action,int* Module)
{
  int delay = 0;
  if (sscanf(action,"%d",&delay) == 1 && delay >= 0)
    /* user enters in ms, we store in 10 ms */
    c10msDelaysBeforePopup = delay/10;
  else 
    fvwm_msg(ERR,"SetMenuDelay","Improper ms delay count as argument.");
}
