/****************************************************************************
 * This module is all new
 * by Rob Nation
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 ****************************************************************************/

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

/*****************************************************************************
 *
 * Waits Scr.ClickTime, or until it is evident that the user is not
 * clicking, but is moving the cursor
 *
 ****************************************************************************/
Bool IsClick(int x,int y,unsigned EndMask, XEvent *d)
{
  int xcurrent,ycurrent,total = 0;
  Time t0;
  extern Time lastTimestamp;

  xcurrent = x;
  ycurrent = y;
  t0 = lastTimestamp;

  while((total < Scr.ClickTime)&&
	(x - xcurrent < 3)&&(x - xcurrent > -3)&&
        (y - ycurrent < 3)&&(y - ycurrent > -3)&&
	((lastTimestamp - t0) < Scr.ClickTime))
    {
      usleep(20000);
      total+=20;
      if(XCheckMaskEvent (dpy,EndMask, d))
	{
	  StashEventTime(d);
	    return True;
	}
      if(XCheckMaskEvent (dpy,ButtonMotionMask|PointerMotionMask, d))
	{
	  xcurrent = d->xmotion.x_root;
	  ycurrent = d->xmotion.y_root;
	  StashEventTime(d);
	}
    }
  return False;
}

/*****************************************************************************
 *
 * Builtin which determines if the button press was a click or double click...
 *
 ****************************************************************************/
void ComplexFunction(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action, int *Module)
{
  char type = MOTION;
  char c;
  MenuItem *mi;
  Bool Persist = False;
  Bool HaveDoubleClick = False;
  Bool NeedsTarget = False;
  char *arguments[10], *junk, *taction;
  int x, y ,i;
  XEvent d, *ev;
  MenuRoot *mr;
  extern Bool desperate;

  mr = FindPopup(action);
  if(mr == NULL)
    {
      if(!desperate)
	fvwm_msg(ERR,"ComplexFunction","No such function %s",action);
      return;
    }
  desperate = 0;
  /* Get the argument list */
  /* First entry in action is the function-name, ignore it */
  action = GetNextToken(action,&junk);
  if(junk != NULL)
    free(junk);
  for(i=0;i<10;i++)
    action = GetNextToken(action,&arguments[i]);
  /* see functions.c to find out which functions need a window to operate on */
  ev = eventp;
  /* In case we want to perform an action on a button press, we
   * need to fool other routines */
  if(eventp->type == ButtonPress)
    eventp->type = ButtonRelease;
  mi = mr->first;
  while(mi != NULL)
    {
      /* make lower case */
      c = *(mi->item);
      NeedsTarget = mi->func_needs_window;
      if(isupper(c))
	c=tolower(c);
      if(c==DOUBLE_CLICK)
	{
	  HaveDoubleClick = True;
	  Persist = True;
	}
      else if(c == IMMEDIATE)
	{
	  if(tmp_win)
	    w = tmp_win->frame;
	  else
	    w = None;
	  taction = expand(mi->action,arguments,tmp_win);
	  ExecuteFunction(taction,tmp_win,eventp,context,-2);
	  free(taction);
	}
      else
	Persist = True;
      mi = mi->next;
    }

  if(!Persist)
    {
      for(i=0;i<10;i++)
	  if(arguments[i] != NULL)free(arguments[i]);
      return;
    }

  /* Only defer execution if there is a possibility of needing
   * a window to operate on */
  if(NeedsTarget)
    {
      if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonPress))
	{
	  WaitForButtonsUp();
	  for(i=0;i<10;i++)
	    if(arguments[i] != NULL)free(arguments[i]);
	  return;
	}
    }

  if(!GrabEm(SELECT))
    {
      XBell(dpy, 0);
      for(i=0;i<10;i++)
	  if(arguments[i] != NULL)free(arguments[i]);
      return;
    }
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		&x,&y,&JunkX, &JunkY, &JunkMask);


  /* Wait and see if we have a click, or a move */
  /* wait 100 msec, see if the user releases the button */
  if(IsClick(x,y,ButtonReleaseMask,&d))
    {
      ev = &d;
      type = CLICK;
    }

  /* If it was a click, wait to see if its a double click */
  if((HaveDoubleClick) && (type == CLICK) &&
     (IsClick(x,y,ButtonPressMask, &d)))
    {
      type = ONE_AND_A_HALF_CLICKS;
      ev = &d;
    }
  if((HaveDoubleClick) && (type == ONE_AND_A_HALF_CLICKS) &&
     (IsClick(x,y,ButtonReleaseMask, &d)))
    {
      type = DOUBLE_CLICK;
      ev = &d;
    }
  /* some functions operate on button release instead of
   * presses. These gets really weird for complex functions ... */
  if(ev->type == ButtonPress)
    ev->type = ButtonRelease;

  mi = mr->first;
  while(mi != NULL)
    {
      /* make lower case */
      c = *(mi->item);
      if(isupper(c))
	c=tolower(c);
      if(c == type)
	{
	  if(tmp_win)
	    w = tmp_win->frame;
	  else
	    w = None;
	  taction = expand(mi->action,arguments,tmp_win);
	  ExecuteFunction(taction,tmp_win,ev,context,-2);
	  free(taction);
	}
      mi = mi->next;
    }
  WaitForButtonsUp();
  UngrabEm();
  for(i=0;i<10;i++)
    if(arguments[i] != NULL)free(arguments[i]);
}


char *expand(char *input, char *arguments[],FvwmWindow *tmp_win)
{
  int l,i,l2,n,k,j;
  char *out;

  l = strlen(input);
  l2 = l;

  i=0;
  while(i<l)
    {
      if(input[i] == '$')
	{
	  n = input[i+1] - '0';
	  if((n >= 0)&&(n <= 9)&&(arguments[n] != NULL))
	    {
	      l2 += strlen(arguments[n])-2;
	      i++;
	    }
	  else if(input[i+1]=='w' || input[i+1] == 'd')
	    {
	      l2 += 16;
	      i++;
	    }
	}
      i++;
    }

  out = safemalloc(l2+1);
  i=0;
  j=0;
  while(i<l)
    {
      if(input[i] == '$')
	{
	  n = input[i+1] - '0';
	  if((n >= 0)&&(n <= 9)&&(arguments[n] != NULL))
	    {
	      for(k=0;k<strlen(arguments[n]);k++)
		out[j++] = arguments[n][k];
	      i++;
	    }
	  else if(input[i+1] == 'w')
	    {
	      if(tmp_win)
		sprintf(&out[j],"0x%x",(unsigned int)tmp_win->w);
	      else
		sprintf(&out[j],"$w");
	      j += strlen(&out[j]);
	      i++;
	    }
	  else if(input[i+1] == 'd')
	    {
	      sprintf(&out[j], "%d", Scr.CurrentDesk);
	      j += strlen(&out[j]);
	      i++;
	    }
	  else if(input[i+1] == '$')
	    {
	      out[j++] = '$';
	      i++;
	    }
	  else
	    out[j++] = input[i];
	}
      else
	out[j++] = input[i];
      i++;
    }
  out[j] = 0;
  return out;
}
