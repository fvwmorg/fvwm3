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
#include <sys/time.h>
#include <signal.h>
#include <stdarg.h>

#include "fvwm.h"
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

FvwmWindow *FocusOnNextTimeStamp = NULL;

char NoName[] = "Untitled"; /* name if no name in XA_WM_NAME */
char NoClass[] = "NoClass"; /* Class if no res_class in class hints */
char NoResource[] = "NoResource"; /* Class if no res_name in class hints */

/**************************************************************************
 *
 * Releases dynamically allocated space used to store window/icon names
 *
 **************************************************************************/
void free_window_names (FvwmWindow *tmp, Bool nukename, Bool nukeicon)
{
  if (!tmp)
    return;

  if (nukename && tmp->name)
    {
      if (tmp->name != tmp->icon_name && tmp->name != NoName)
	XFree(tmp->name);
      tmp->name = NULL;
    }
  if (nukeicon && tmp->icon_name)
    {
      if (tmp->name != tmp->icon_name && tmp->icon_name != NoName)
	XFree(tmp->icon_name);
      tmp->icon_name = NULL;
    }

  return;
}

/***************************************************************************
 *
 * Handles destruction of a window
 *
 ****************************************************************************/
void Destroy(FvwmWindow *Tmp_win)
{
  int i;
  extern FvwmWindow *ButtonWindow;
  extern FvwmWindow *colormap_win;
  extern Boolean PPosOverride;

  /*
   * Warning, this is also called by HandleUnmapNotify; if it ever needs to
   * look at the event, HandleUnmapNotify will have to mash the UnmapNotify
   * into a DestroyNotify.
   */
  if(!Tmp_win)
    return;

  /* first of all, remove the window from the list of all windows! */
  /*
      RBW - 11/13/1998 - new: have to unhook the stacking order chain also.
      There's always a prev and next, since this is a ring anchored on
      Scr.FvwmRoot
  */
  Tmp_win->stack_prev->stack_next = Tmp_win->stack_next;
  Tmp_win->stack_next->stack_prev = Tmp_win->stack_prev;

  Tmp_win->prev->next = Tmp_win->next;
  if (Tmp_win->next != NULL)
    Tmp_win->next->prev = Tmp_win->prev;

  XUnmapWindow(dpy, Tmp_win->frame);

  if(!PPosOverride)
    XSync(dpy,0);

  if(Tmp_win == Scr.Hilite)
    Scr.Hilite = NULL;

  BroadcastPacket(M_DESTROY_WINDOW, 3,
                  Tmp_win->w, Tmp_win->frame, (unsigned long)Tmp_win);

  if(Scr.PreviousFocus == Tmp_win)
    Scr.PreviousFocus = NULL;

  if(ButtonWindow == Tmp_win)
    ButtonWindow = NULL;

  if((Tmp_win == Scr.Focus)&&(HAS_CLICK_FOCUS(Tmp_win)))
    {
      if(Tmp_win->next)
	{
	  HandleHardFocus(Tmp_win->next);
	}
      else
	SetFocus(Scr.NoFocusWin, NULL,1);
    }
  else if(Scr.Focus == Tmp_win)
    SetFocus(Scr.NoFocusWin, NULL,1);

  if(Tmp_win == FocusOnNextTimeStamp)
    FocusOnNextTimeStamp = NULL;

  if(Tmp_win == Scr.Ungrabbed)
    Scr.Ungrabbed = NULL;

  if(Tmp_win == Scr.pushed_window)
    Scr.pushed_window = NULL;

  if(Tmp_win == colormap_win)
    colormap_win = NULL;

  XDestroyWindow(dpy, Tmp_win->frame);
  XDeleteContext(dpy, Tmp_win->frame, FvwmContext);

  XDestroyWindow(dpy, Tmp_win->Parent);

  XDeleteContext(dpy, Tmp_win->Parent, FvwmContext);

  XDeleteContext(dpy, Tmp_win->w, FvwmContext);

  if ((Tmp_win->icon_w)&&(IS_PIXMAP_OURS(Tmp_win)))
    {
      XFreePixmap(dpy, Tmp_win->iconPixmap);
      XFreePixmap(dpy, Tmp_win->icon_maskPixmap);
    }

#ifdef MINI_ICON
  if (Tmp_win->mini_icon)
    {
      DestroyPicture(dpy, Tmp_win->mini_icon);
    }
#endif

  if (Tmp_win->icon_w)
    {
      XDestroyWindow(dpy, Tmp_win->icon_w);
      XDeleteContext(dpy, Tmp_win->icon_w, FvwmContext);
    }
  if((IS_ICON_OURS(Tmp_win))&&(Tmp_win->icon_pixmap_w != None))
    XDestroyWindow(dpy, Tmp_win->icon_pixmap_w);
  if(Tmp_win->icon_pixmap_w != None)
    XDeleteContext(dpy, Tmp_win->icon_pixmap_w, FvwmContext);

  if (HAS_TITLE(Tmp_win))
    {
      XDeleteContext(dpy, Tmp_win->title_w, FvwmContext);
      for(i=0;i<Scr.nr_left_buttons;i++)
	XDeleteContext(dpy, Tmp_win->left_w[i], FvwmContext);
      for(i=0;i<Scr.nr_right_buttons;i++)
	if(Tmp_win->right_w[i] != None)
	  XDeleteContext(dpy, Tmp_win->right_w[i], FvwmContext);
    }
  if (HAS_BORDER(Tmp_win))
    {
      for(i=0;i<4;i++)
	XDeleteContext(dpy, Tmp_win->sides[i], FvwmContext);
      for(i=0;i<4;i++)
	XDeleteContext(dpy, Tmp_win->corners[i], FvwmContext);
    }

  free_window_names (Tmp_win, True, True);
  if (Tmp_win->wmhints)
    XFree ((char *)Tmp_win->wmhints);
  /* removing NoClass change for now... */
#if 0
  if (Tmp_win->class.res_name)
    XFree ((char *)Tmp_win->class.res_name);
  if (Tmp_win->class.res_class)
    XFree ((char *)Tmp_win->class.res_class);
#else
  if (Tmp_win->class.res_name && Tmp_win->class.res_name != NoResource)
    XFree ((char *)Tmp_win->class.res_name);
  if (Tmp_win->class.res_class && Tmp_win->class.res_class != NoClass)
    XFree ((char *)Tmp_win->class.res_class);
#endif /* 0 */
  if(Tmp_win->mwm_hints)
    XFree((char *)Tmp_win->mwm_hints);

  if(Tmp_win->cmap_windows != (Window *)NULL)
    XFree((void *)Tmp_win->cmap_windows);

  /*  Recapture reuses this struct, so don't free it.  */
  if (!DO_REUSE_DESTROYED(Tmp_win))
    {
      free((char *)Tmp_win);
    }

  if(!PPosOverride)
    XSync(dpy,0);
  return;
}



/**************************************************************************
 *
 * Removes expose events for a specific window from the queue
 *
 *************************************************************************/
int flush_expose (Window w)
{
  XEvent dummy;
  int i=0;

  while (XCheckTypedWindowEvent (dpy, w, Expose, &dummy))i++;
  return i;
}



/***********************************************************************
 *
 *  Procedure:
 *	RestoreWithdrawnLocation
 *
 *  Puts windows back where they were before fvwm took over
 *
 ************************************************************************/
void RestoreWithdrawnLocation (FvwmWindow *tmp,Bool restart)
{
  int a,b,w2,h2;
  unsigned int mask;
  XWindowChanges xwc;

  if(!tmp)
    return;

  if (XGetGeometry (dpy, tmp->w, &JunkRoot, &xwc.x, &xwc.y,
		    &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth))
    {
      XTranslateCoordinates(dpy,tmp->frame,Scr.Root,xwc.x,xwc.y,
			    &a,&b,&JunkChild);
      xwc.x = a + tmp->xdiff;
      xwc.y = b + tmp->ydiff;
      xwc.border_width = tmp->old_bw;
      mask = (CWX | CWY| CWBorderWidth);

      /* We can not assume that the window is currently on the screen.
       * Although this is normally the case, it is not always true.  The
       * most common example is when the user does something in an
       * application which will, after some amount of computational delay,
       * cause the window to be unmapped, but then switches screens before
       * this happens.  The XTranslateCoordinates call above will set the
       * window coordinates to either be larger than the screen, or negative.
       * This will result in the window being placed in odd, or even
       * unviewable locations when the window is remapped.  The followin code
       * forces the "relative" location to be within the bounds of the display.
       *
       * gpw -- 11/11/93
       *
       * Unfortunately, this does horrendous things during re-starts,
       * hence the "if(restart)" clause (RN)
       *
       * Also, fixed so that it only does this stuff if a window is more than
       * half off the screen. (RN)
       */

      if(!restart)
	{
	  /* Don't mess with it if its partially on the screen now */
	  if((tmp->frame_x < 0)||(tmp->frame_y<0)||
	     (tmp->frame_x >= Scr.MyDisplayWidth)||
	     (tmp->frame_y >= Scr.MyDisplayHeight))
	    {
	      w2 = (tmp->frame_width>>1);
	      h2 = (tmp->frame_height>>1);
	      if (( xwc.x < -w2) || (xwc.x > (Scr.MyDisplayWidth-w2 )))
		{
		  xwc.x = xwc.x % Scr.MyDisplayWidth;
		  if ( xwc.x < -w2 )
		    xwc.x += Scr.MyDisplayWidth;
		}
	      if ((xwc.y < -h2) || (xwc.y > (Scr.MyDisplayHeight-h2 )))
		{
		  xwc.y = xwc.y % Scr.MyDisplayHeight;
		  if ( xwc.y < -h2 )
		    xwc.y += Scr.MyDisplayHeight;
		}
	    }
	}
      XReparentWindow (dpy, tmp->w,Scr.Root,xwc.x,xwc.y);

      if(IS_ICONIFIED(tmp) && !IS_ICON_SUPPRESSED(tmp))
	{
	  if (tmp->icon_w)
	    XUnmapWindow(dpy, tmp->icon_w);
	  if (tmp->icon_pixmap_w)
	    XUnmapWindow(dpy, tmp->icon_pixmap_w);
	}

      XConfigureWindow (dpy, tmp->w, mask, &xwc);
      if(!restart)
	XSync(dpy,0);
    }
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
  /* Only update is the new timestamp is later than the old one, or
   * if the new one is from a time at least 30 seconds earlier than the
   * old one (in which case the system clock may have changed) */
  if((NewTimestamp > lastTimestamp)||((lastTimestamp - NewTimestamp) > 30000))
    lastTimestamp = NewTimestamp;
  if(FocusOnNextTimeStamp)
    {
      SetFocus(FocusOnNextTimeStamp->w,FocusOnNextTimeStamp,1);
      FocusOnNextTimeStamp = NULL;
    }
  return True;
}


void ComputeActualPosition(int x,int y,int x_unit,int y_unit,
			   int width,int height,int *pfinalX, int *pfinalY)
{
  *pfinalX = x*x_unit/100;
  *pfinalY = y*y_unit/100;
  if (*pfinalX < 0)
    *pfinalX += Scr.MyDisplayWidth - width;
  if (*pfinalY < 0)
    *pfinalY += Scr.MyDisplayHeight - height;
}

int GetTwoArguments(char *action, int *val1, int *val2, int *val1_unit,
		    int *val2_unit)
{
  *val1_unit = Scr.MyDisplayWidth;
  *val2_unit = Scr.MyDisplayHeight;
  return GetTwoPercentArguments(action, val1, val2, val1_unit, val2_unit);
}

/* The vars are named for the x-direction, but this is used for both x and y */
static
int GetOnePositionArgument(char *s1,int x,int w,int *pFinalX,float factor,
			   int max)
{
  int val;
  int cch = strlen(s1);

  if (cch == 0)
    return 0;
  if (s1[cch-1] == 'p') {
    factor = 1;  /* Use pixels, so don't multiply by factor */
    s1[cch-1] = '\0';
  }
  if (strcmp(s1,"w") == 0) {
    *pFinalX = x;
  } else if (sscanf(s1,"w-%d",&val) == 1) {
    *pFinalX = x-(val*factor);
  } else if (sscanf(s1,"w+%d",&val) == 1) {
    *pFinalX = x+(val*factor);
  } else if (sscanf(s1,"-%d",&val) == 1) {
    *pFinalX = max-w - val*factor;
  } else if (sscanf(s1,"%d",&val) == 1) {
    *pFinalX = val*factor;
  } else {
    return 0;
  }
  /* DEBUG_FPRINTF((stderr,"Got %d\n",*pFinalX)); */
  return 1;
}

/* GetMoveArguments is used for Move & AnimatedMove
 * It lets you specify in all the following ways
 *   20  30          Absolute percent position, from left edge and top
 *  -50  50          Absolute percent position, from right edge and top
 *   10p 5p          Absolute pixel position
 *   10p -0p         Absolute pixel position, from bottom
 *  w+5  w-10p       Relative position, right 5%, up ten pixels
 * Returns 2 when x & y have parsed without error, 0 otherwise
 */
int GetMoveArguments(char *action, int x, int y, int w, int h,
                     int *pFinalX, int *pFinalY, Bool *fWarp)
{
  char *s1, *s2, *warp;
  int scrWidth = Scr.MyDisplayWidth;
  int scrHeight = Scr.MyDisplayHeight;
  int retval = 0;

  action = GetNextToken(action, &s1);
  action = GetNextToken(action, &s2);
  GetNextToken(action, &warp);
  *fWarp = StrEquals(warp, "Warp");

  if (s1 != NULL && s2 != NULL) {
    if (GetOnePositionArgument(s1,x,w,pFinalX,(float)scrWidth/100,scrWidth) &&
        GetOnePositionArgument(s2,y,h,pFinalY,(float)scrHeight/100,scrHeight))
      retval = 2;
    else
	*fWarp = FALSE; /* make sure warping is off for interactive moves */
  }

  if (s1)
    free(s1);
  if (s2)
    free(s2);
  if (warp)
    free(warp);

  return retval;
}

/***************************************************************************
 *
 * Wait for all mouse buttons to be released
 * This can ease some confusion on the part of the user sometimes
 *
 * Discard superflous button events during this wait period.
 *
 ***************************************************************************/
void WaitForButtonsUp(void)
{
  Bool AllUp = False;
  XEvent JunkEvent;
  unsigned int mask;

  while(!AllUp)
    {
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
      XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &JunkX, &JunkY, &JunkX, &JunkY, &mask);

      if((mask&
	  (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask))==0)
	AllUp = True;
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
 * Grab the pointer and keyboard
 *
 ****************************************************************************/
Bool GrabEm(int cursor)
{
  int i=0,val=0;
  unsigned int mask;

  XSync(dpy,0);
  /* move the keyboard focus prior to grabbing the pointer to
   * eliminate the enterNotify and exitNotify events that go
   * to the windows */
  if(Scr.PreviousFocus == NULL)
    Scr.PreviousFocus = Scr.Focus;
  SetFocus(Scr.NoFocusWin,NULL,0);
  mask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|PointerMotionMask
    | EnterWindowMask | LeaveWindowMask;
  while((i<1000)&&(val=XGrabPointer(dpy, Scr.Root, True, mask,
				    GrabModeAsync, GrabModeAsync, Scr.Root,
				    Scr.FvwmCursors[cursor], CurrentTime)!=
		   GrabSuccess))
    {
      i++;
      /* If you go too fast, other windows may not get a change to release
       * any grab that they have. */
      usleep(1000);
    }

  /* If we fall out of the loop without grabbing the pointer, its
     time to give up */
  XSync(dpy,0);
  if(val!=GrabSuccess)
    {
      return False;
    }
  return True;
}


/*****************************************************************************
 *
 * UnGrab the pointer and keyboard
 *
 ****************************************************************************/
void UngrabEm(void)
{
  Window w;

  XSync(dpy,0);
  XUngrabPointer(dpy,CurrentTime);

  if(Scr.PreviousFocus != NULL)
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


/**************************************************************************
 *
 * Unmaps a window on transition to a new desktop
 *
 *************************************************************************/
void UnmapIt(FvwmWindow *t)
{
  XWindowAttributes winattrs;
  unsigned long eventMask;
  /*
   * Prevent the receipt of an UnmapNotify, since that would
   * cause a transition to the Withdrawn state.
   */
  XGetWindowAttributes(dpy, t->w, &winattrs);
  eventMask = winattrs.your_event_mask;
  XSelectInput(dpy, t->w, eventMask & ~StructureNotifyMask);
  if(IS_ICONIFIED(t))
    {
      if(t->icon_pixmap_w != None)
	XUnmapWindow(dpy,t->icon_pixmap_w);
      if(t->icon_w != None)
	XUnmapWindow(dpy,t->icon_w);
    }
  else if(IS_MAPPED(t) || IS_MAP_PENDING(t))
    {
      XUnmapWindow(dpy,t->frame);
    }
  XSelectInput(dpy, t->w, eventMask);
}

/**************************************************************************
 *
 * Maps a window on transition to a new desktop
 *
 *************************************************************************/
void MapIt(FvwmWindow *t)
{
  if(IS_ICONIFIED(t))
    {
      if(t->icon_pixmap_w != None)
	XMapWindow(dpy,t->icon_pixmap_w);
      if(t->icon_w != None)
	XMapWindow(dpy,t->icon_w);
    }
  else if(IS_MAPPED(t))
    {
      XMapWindow(dpy,t->frame);
      SET_MAP_PENDING(t, 1);
      XMapWindow(dpy, t->Parent);
   }
}

/* send RESTACK packets for all windows between s1 and s2 */
void BroadcastRestack (FvwmWindow *s1, FvwmWindow *s2)
{
  FvwmWindow *t, *t2;
  int num, i;
  unsigned long *body, *bp, length;
  extern Time lastTimestamp;

  if (s1 == &Scr.FvwmRoot)
   {
      t = s1->stack_next;
      /* t has been moved to the top of stack */

      BroadcastPacket (M_RAISE_WINDOW, 3, t->w, t->frame, (unsigned long)t);
      if (t->stack_next == s2)
        {
          /* avoid sending empty RESTACK packet */
          return;
        }
   }
  else
   {
      t = s1;
   }
  for (t2 = t, num = 1 ; t2 != s2; t2 = t2->stack_next, num++)
    ;

  length = HEADER_SIZE + 3*num;
  body = (unsigned long *) safemalloc (length*sizeof(unsigned long));

  bp = body;
  *(bp++) = START_FLAG;
  *(bp++) = M_RESTACK;
  *(bp++) = length;
  *(bp++) = lastTimestamp;
  for (t2 = t; t2 != s2; t2 = t2->stack_next)
   {
      *(bp++) = t2->w;
      *(bp++) = t2->frame;
      *(bp++) = (unsigned long)t2;
   }
   for (i = 0; i < npipes; i++)
     PositiveWrite(i, body, length*sizeof(unsigned long));

   free (body);
}

/*
   Raise t and its transients to the top of its layer.
   For the pager to work properly it is necessary that
   RaiseWindow *always* sends a proper M_RESTACK packet,
   even if the stacking order didn't change.
*/
void RaiseWindow(FvwmWindow *t)
{
  FvwmWindow *s, *r, *t2, *next, tmp_r, tmp_s;
  unsigned int flags;
  int i, count;
  XWindowChanges changes;
  Window *wins;

  Scr.LastWindowRaised = t;

  /* detach t early, so it doesn't make trouble in the loops */
  t->stack_prev->stack_next = t->stack_next;
  t->stack_next->stack_prev = t->stack_prev;

  count = 1;
  if (IS_ICONIFIED(t) && !IS_ICON_SUPPRESSED(t))
    {
      count += 2;
    }

#ifndef DONT_RAISE_TRANSIENTS
  /* collect the transients in a temp list */
  tmp_s.stack_prev = &tmp_r;
  tmp_r.stack_next = &tmp_s;
  for (t2 = Scr.FvwmRoot.stack_next; t2 != &Scr.FvwmRoot; t2 = next)
    {
      next = t2->stack_next;
      if ((IS_TRANSIENT(t2)) && (t2->transientfor == t->w) &&
	  (t2->layer == t->layer))
        {
          /* t2 is a transient to raise */
          count++;
          if (IS_ICONIFIED(t2) && !IS_ICON_SUPPRESSED(t2))
            {
	      count += 2;
            }

          /* unplug it */
          t2->stack_next->stack_prev = t2->stack_prev;
          t2->stack_prev->stack_next = t2->stack_next;

          /* put it above tmp_s */
          t2->stack_next = &tmp_s;
          t2->stack_prev = tmp_s.stack_prev;
          t2->stack_next->stack_prev = t2;
          t2->stack_prev->stack_next = t2;
	}
    }
#endif /* DONT_RAISE_TRANSIENTS */

  /* now find the place to reinsert t and friends */
  for (s = Scr.FvwmRoot.stack_next; s != &Scr.FvwmRoot; s = s->stack_next)
    {
      if (t->layer >= s->layer)
	{
	  break;
        }
    }
  r = s->stack_prev;

#ifndef DONT_RAISE_TRANSIENTS
  /* insert all transients between r and s. */
  r->stack_next = tmp_r.stack_next;
  r->stack_next->stack_prev = r;
  s->stack_prev = tmp_s.stack_prev;
  s->stack_prev->stack_next = s;
#endif /* DONT_RAISE_TRANSIENTS */

  /* don't forget t itself */
  t->stack_next = s;
  t->stack_prev = s->stack_prev;
  t->stack_prev->stack_next = t;
  t->stack_next->stack_prev = t;

  wins = (Window*) safemalloc (count * sizeof (Window));

  i = 0;
  for (t2 = r->stack_next; t2 != s; t2 = t2->stack_next)
    {
      if (i >= count) {
	fvwm_msg (ERR, "RaiseWindow", "more transients than expected");
	break;
      }
      wins[i++] = t2->frame;
      if (IS_ICONIFIED(t2) && !IS_ICON_SUPPRESSED(t2))
	{
	  if(t2->icon_w != None)
	    wins[i++] = t2->icon_w;
	  if(t2->icon_pixmap_w != None)
	    wins[i++] = t2->icon_pixmap_w;
	}
    }

  changes.sibling = s->frame;
  if (changes.sibling != None)
    {
      changes.stack_mode = Above;
      flags = CWSibling|CWStackMode;
    }
  else
    {
      changes.stack_mode = Below;
      flags = CWStackMode;
    }

  XConfigureWindow (dpy, r->stack_next->frame, flags, &changes);
  XRestackWindows (dpy, wins, count);

  /* send out (one or more) M_RESTACK packets for windows between r and s */
  BroadcastRestack (r, s);

  free (wins);

  raisePanFrames();

  /*
     The following is a hack to raise X windows over native windows
     which is needed for some (all ?) X servers running under windows NT.
   */
  if (Scr.go.RaiseHackNeeded)
    {
      Window junk;
      Window *tops;
      int i, num;
      Bool found = False;

      /* get *all* toplevels (even including override_redirects) */
      XQueryTree (dpy, Scr.Root, &junk, &junk, &tops, &num);

      /* raise from tmp_win upwards to get them above NT windows */
      for (i = 0; i < num; i++) {
        if (tops[i] == t->frame) found = True;
        if (found) XRaiseWindow (dpy, tops[i]);
      }

      XFree (tops);
    }
}


void LowerWindow(FvwmWindow *t)
{
  FvwmWindow *s;
  XWindowChanges changes;
  unsigned int flags;

  Scr.LastWindowRaised = NULL;

  for (s = Scr.FvwmRoot.stack_next; s != &Scr.FvwmRoot; s = s->stack_next)
    {
      if (t->layer > s->layer)
        {
	  break;
        }
    }

  if (s == t->stack_next)
    {
      return;
    }


  t->stack_prev->stack_next = t->stack_next;
  t->stack_next->stack_prev = t->stack_prev;

  t->stack_next = s;
  t->stack_prev = s->stack_prev;
  t->stack_prev->stack_next = t;
  t->stack_next->stack_prev = t;

  changes.sibling = s->frame;
  if (changes.sibling != None)
    {
      changes.stack_mode = Above;
      flags = CWSibling|CWStackMode;
    }
  else
    {
      changes.stack_mode = Below;
      flags = CWStackMode;
    }

  XConfigureWindow(dpy, t->frame, flags, &changes);

  if (IS_ICONIFIED(t) && !IS_ICON_SUPPRESSED(t))
    {
      changes.sibling = t->frame;
      changes.stack_mode = Below;
      flags = CWSibling|CWStackMode;
      if (t->icon_w != None)
	XConfigureWindow(dpy, t->icon_w, flags, &changes);
      if (t->icon_pixmap_w != None)
	XConfigureWindow(dpy, t->icon_pixmap_w, flags, &changes);
    }

  BroadcastRestack (t->stack_prev, t->stack_next);
}


void HandleHardFocus(FvwmWindow *t)
{
  int x,y;

  FocusOnNextTimeStamp = t;
  Scr.Focus = NULL;
  /* Do something to guarantee a new time stamp! */
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		&JunkX, &JunkY, &x, &y, &JunkMask);
  GrabEm(WAIT);
  XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
	       Scr.MyDisplayHeight,
	       x + 2,y+2);
  XSync(dpy,0);
  XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
	       Scr.MyDisplayHeight,
	       x ,y);
  UngrabEm();
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
    if (XFindContext(dpy, child, FvwmContext, (caddr_t *) &Tmp_win) ==
XCNOENT)
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
  XChangeGC(dpy,Scr.ScratchGC3,Globalgcm,&Globalgcv);
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

Bool IsWindowOnThisPage(FvwmWindow *fw)
{
  return (fw->Desk == Scr.CurrentDesk &&
	  fw->frame_x > -fw->frame_width &&
	  fw->frame_x < Scr.MyDisplayWidth &&
	  fw->frame_y > -fw->frame_height &&
	  fw->frame_y < Scr.MyDisplayHeight) ?
    True : False;
}
