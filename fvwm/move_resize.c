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

/*****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/************************************************************************
 *
 * code for moving and resizing windows
 *
 ***********************************************************************/

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <X11/keysym.h>
#include "fvwm.h"
#include "events.h"
#include "misc.h"
#include "screen.h"
#include "module_interface.h"
#include "move_resize.h"
#include "virtual.h"
#include "borders.h"
#include "colormaps.h"
#include "decorations.h"

/* ----- move globals ----- */
extern XEvent Event;
Bool NeedToResizeToo;

/* Animated move stuff added by Greg J. Badros, gjb@cs.washington.edu */

float rgpctMovementDefault[32] = {
    -.01, 0, .01, .03,.08,.18,.3,.45,.60,.75,.85,.90,.94,.97,.99,1.0
    /* must end in 1.0 */
  };

int cmsDelayDefault = 10; /* milliseconds */
static void DisplayPosition(FvwmWindow *, int, int,Bool);
/* ----- end of move globals ----- */

/* ----- resize globals ----- */

/* DO NOT USE (STATIC) GLOBALS IN THIS MODULE!
 * Since some functions are called from other modules unwanted side effects
 * (i.e. bugs.) would be created */

extern Window PressedW;

static void DoResize(int x_root, int y_root, FvwmWindow *tmp_win,
		     rectangle *drag, rectangle *orig, int *xmotionp,
		     int *ymotionp);
static void DisplaySize(FvwmWindow *, int, int, Bool, Bool);
/* ----- end of resize globals ----- */

/* The vars are named for the x-direction, but this is used for both x and y */
static int GetOnePositionArgument(
  char *s1,int x,int w,int *pFinalX,float factor, int max)
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
static int GetMoveArguments(char *action, int x, int y, int w, int h,
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

  if (s1 != NULL && s2 != NULL)
  {
    if (GetOnePositionArgument(s1,x,w,pFinalX,(float)scrWidth/100,scrWidth) &&
        GetOnePositionArgument(s2,y,h,pFinalY,(float)scrHeight/100,scrHeight))
      retval = 2;
    else
      *fWarp = FALSE; /* make sure warping is off for interactive moves */
  }
  /* Begine code ---cpatil*/
  else
  {
    /* not enough arguments, switch to current page. */
    while (*pFinalX < 0)
    {
      *pFinalX = Scr.MyDisplayWidth + *pFinalX;
    }
    while (*pFinalY < 0)
    {
      *pFinalY = Scr.MyDisplayHeight + *pFinalY;
    }
  }

  if (s1)
    free(s1);
  if (s2)
    free(s2);
  if (warp)
    free(warp);

  return retval;
}

static void InteractiveMove(Window *win, FvwmWindow *tmp_win, int *FinalX,
			    int *FinalY, XEvent *eventp)
{
  int origDragX,origDragY,DragX, DragY, DragWidth, DragHeight;
  int XOffset, YOffset;
  Window w;
  Bool opaque_move = False;

  w = *win;

  InstallRootColormap();
  /* warp the pointer to the cursor position from before menu appeared*/
  /* domivogt (17-May-1999): an XFlush should not hurt anyway, so do it
   * unconditionally to remove the external */
  XFlush(dpy);

  /* Although a move is usually done with a button depressed we have to check
   * for ButtonRelease too since the event may be faked. */
  GetLocationFromEventOrQuery(dpy, Scr.Root, &Event, &DragX, &DragY);

  if(!GrabEm(CRS_MOVE))
  {
    XBell(dpy, 0);
    return;
  }

  XGetGeometry(dpy, w, &JunkRoot, &origDragX, &origDragY,
	       (unsigned int *)&DragWidth, (unsigned int *)&DragHeight,
	       &JunkBW,  &JunkDepth);

  if(DragWidth*DragHeight <
     (Scr.OpaqueSize*Scr.MyDisplayWidth*Scr.MyDisplayHeight)/100)
    opaque_move = True;
  else
    MyXGrabServer(dpy);

  if((!opaque_move)&&(IS_ICONIFIED(tmp_win)))
    XUnmapWindow(dpy,w);

  XOffset = origDragX - DragX;
  YOffset = origDragY - DragY;
  XMapRaised(dpy,Scr.SizeWindow);
  moveLoop(tmp_win, XOffset,YOffset,DragWidth,DragHeight, FinalX,FinalY,
	   opaque_move,False);

  XUnmapWindow(dpy,Scr.SizeWindow);
  UninstallRootColormap();

  if(!opaque_move)
    MyXUngrabServer(dpy);
  UngrabEm();
}


/* Perform the movement of the window. ppctMovement *must* have a 1.0 entry
 * somewhere in ins list of floats, and movement will stop when it hits a 1.0
 * entry */
static void AnimatedMoveAnyWindow(FvwmWindow *tmp_win, Window w, int startX,
				  int startY, int endX, int endY,
				  Bool fWarpPointerToo, int cmsDelay,
				  float *ppctMovement )
{
  int pointerX, pointerY;
  int currentX, currentY;
  int lastX, lastY;
  int deltaX, deltaY;

  if (tmp_win && IS_FIXED(tmp_win))
    return;

  /* set our defaults */
  if (ppctMovement == NULL)
    ppctMovement = rgpctMovementDefault;
  if (cmsDelay < 0)
    cmsDelay = cmsDelayDefault;

  if (startX < 0 || startY < 0)
  {
    XGetGeometry(dpy, w, &JunkRoot, &currentX, &currentY,
		 &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
    if (startX < 0)
      startX = currentX;
    if (startY < 0)
      startY = currentY;
  }

  deltaX = endX - startX;
  deltaY = endY - startY;
  lastX = startX;
  lastY = startY;

  if (deltaX == 0 && deltaY == 0)
    /* go nowhere fast */
    return;

  /* Needed for aborting */
  XGrabKeyboard(dpy, Scr.Root, False, GrabModeAsync, GrabModeAsync,
		CurrentTime);
  do
  {
    currentX = startX + deltaX * (*ppctMovement);
    currentY = startY + deltaY * (*ppctMovement);
    if (lastX == currentX && lastY == currentY)
      /* don't waste time in the same spot */
      continue;
    XMoveWindow(dpy,w,currentX,currentY);
    if (fWarpPointerToo == TRUE)
    {
      XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &JunkX,&JunkY,&pointerX,&pointerY,&JunkMask);
      pointerX += currentX - lastX;
      pointerY += currentY - lastY;
      XWarpPointer(dpy,None,Scr.Root,0,0,0,0,
		   pointerX,pointerY);
    }
#ifndef DISABLE_CONFIGURE_NOTIFY_DURING_MOVE
    if (tmp_win && !IS_SHADED(tmp_win))
    {
      /* send configure notify event for windows that care about their
       * location */
      XEvent client_event;
      client_event.type = ConfigureNotify;
      client_event.xconfigure.display = dpy;
      client_event.xconfigure.event = tmp_win->w;
      client_event.xconfigure.window = tmp_win->w;
      client_event.xconfigure.x = currentX + tmp_win->boundary_width;
      client_event.xconfigure.y = currentY + tmp_win->title_g.height +
	tmp_win->boundary_width;
      client_event.xconfigure.width = tmp_win->frame_g.width -
	2 * tmp_win->boundary_width;
      client_event.xconfigure.height = tmp_win->frame_g.height -
	2 * tmp_win->boundary_width - tmp_win->title_g.height;
      client_event.xconfigure.border_width = 0;
      /* Real ConfigureNotify events say we're above title window, so ... */
      /* what if we don't have a title ????? */
      client_event.xconfigure.above = tmp_win->frame;
      client_event.xconfigure.override_redirect = False;
      XSendEvent(dpy, tmp_win->w, False, StructureNotifyMask, &client_event);
#ifdef FVWM_DEBUG_MSGS
      fvwm_msg(DBG,"AnimatedMoveAnyWindow",
	       "Sent ConfigureNotify (w == %d, h == %d)",
               client_event.xconfigure.width,client_event.xconfigure.height);
#endif
    }
#endif
    XFlush(dpy);
    if (tmp_win)
    {
      tmp_win->frame_g.x = currentX;
      tmp_win->frame_g.y = currentY;
      BroadcastConfig(M_CONFIGURE_WINDOW, tmp_win);
      FlushOutputQueues();
    }

    usleep(cmsDelay*1000); /* usleep takes microseconds */
    /* this didn't work for me -- maybe no longer necessary since
     * we warn the user when they use > .5 seconds as a between-frame delay
     * time.
     *
     * domivogt (28-apr-1999): That is because the keyboard was not grabbed.
     * works nicely now.
     */
    if (XCheckMaskEvent(dpy,
			ButtonPressMask|ButtonReleaseMask|KeyPressMask,
			&Event))
    {
      StashEventTime(&Event);
      /* finish the move immediately */
      XMoveWindow(dpy,w,endX,endY);
      break;
    }
    lastX = currentX;
    lastY = currentY;
  }
  while (*ppctMovement != 1.0 && ppctMovement++);
  XUngrabKeyboard(dpy, CurrentTime);
  XFlush(dpy);
  return;
}

/* used for moving menus, not a client window */
void AnimatedMoveOfWindow(Window w, int startX, int startY,
                             int endX, int endY, Bool fWarpPointerToo,
                             int cmsDelay, float *ppctMovement)
{
  AnimatedMoveAnyWindow(NULL, w, startX, startY, endX, endY, fWarpPointerToo,
                        cmsDelay, ppctMovement);
}

/* used for moving client windows */
void AnimatedMoveFvwmWindow(FvwmWindow *tmp_win, Window w, int startX,
			    int startY, int endX, int endY,
			    Bool fWarpPointerToo, int cmsDelay,
			    float *ppctMovement)
{
  AnimatedMoveAnyWindow(tmp_win, w, startX, startY, endX, endY,
			fWarpPointerToo, cmsDelay, ppctMovement);
}


/****************************************************************************
 *
 * Start a window move operation
 *
 ****************************************************************************/
void move_window_doit(F_CMD_ARGS, Bool fAnimated, Bool fMoveToPage)
{
  int FinalX, FinalY;
  int n;
  int x,y;
  unsigned int width, height;
  int page_x, page_y;
  Bool fWarp = FALSE;

  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_MOVE,ButtonPress))
    return;

  if (tmp_win == NULL)
    return;

  if (IS_FIXED (tmp_win))
    return;

  /* gotta have a window */
  w = tmp_win->frame;
  if(IS_ICONIFIED(tmp_win))
  {
    if(tmp_win->icon_pixmap_w != None)
    {
      XUnmapWindow(dpy,tmp_win->icon_w);
      w = tmp_win->icon_pixmap_w;
    }
    else
      w = tmp_win->icon_w;
  }

  XGetGeometry(dpy, w, &JunkRoot, &x, &y,
	       &width, &height, &JunkBW, &JunkDepth);
  if (fMoveToPage)
  {
    fAnimated = FALSE;
    FinalX = x % Scr.MyDisplayWidth;
    FinalY = y % Scr.MyDisplayHeight;
    if (get_page_arguments(action, &page_x, &page_y))
    {
      SET_STICKY(tmp_win, 0);
      FinalX += page_x - Scr.Vx;
	  FinalY += page_y - Scr.Vy;
    }
  }
  else
  {
    n = GetMoveArguments(action,x,y,width,height,&FinalX,&FinalY,&fWarp);
    if (n != 2)
      InteractiveMove(&w,tmp_win,&FinalX,&FinalY,eventp);
  }

  if (w == tmp_win->frame)
  {
    if (fAnimated)
    {
      AnimatedMoveFvwmWindow(tmp_win,w,-1,-1,FinalX,FinalY,fWarp,-1,NULL);
    }
    SetupFrame(tmp_win, FinalX, FinalY,
	       tmp_win->frame_g.width, tmp_win->frame_g.height,True,False);
    if (fWarp & !fAnimated)
      XWarpPointer(dpy, None, None, 0, 0, 0, 0, FinalX - x, FinalY - y);
  }
  else /* icon window */
  {
    SET_ICON_MOVED(tmp_win, 1);
    tmp_win->icon_x_loc = FinalX ;
    tmp_win->icon_xl_loc = FinalX -
      (tmp_win->icon_w_width - tmp_win->icon_p_width)/2;
    tmp_win->icon_y_loc = FinalY;
    BroadcastPacket(M_ICON_LOCATION, 7,
		    tmp_win->w, tmp_win->frame,
		    (unsigned long)tmp_win,
		    tmp_win->icon_x_loc, tmp_win->icon_y_loc,
		    tmp_win->icon_p_width,
		    tmp_win->icon_w_height + tmp_win->icon_p_height);
    if (fAnimated)
    {
      AnimatedMoveOfWindow(tmp_win->icon_w,-1,-1,tmp_win->icon_xl_loc,
			   FinalY+tmp_win->icon_p_height, fWarp,-1,NULL);
    }
    else
    {
      XMoveWindow(dpy,tmp_win->icon_w, tmp_win->icon_xl_loc,
		  FinalY+tmp_win->icon_p_height);
      if (fWarp)
	XWarpPointer(dpy, None, None, 0, 0, 0, 0, FinalX - x, FinalY - y);
    }
    if(tmp_win->icon_pixmap_w != None)
    {
      XMapWindow(dpy,tmp_win->icon_w);
      if (fAnimated)
      {
	AnimatedMoveOfWindow(tmp_win->icon_pixmap_w, -1,-1,
			     tmp_win->icon_x_loc,FinalY,fWarp,-1,NULL);
      }
      else
      {
	XMoveWindow(dpy, tmp_win->icon_pixmap_w, tmp_win->icon_x_loc, FinalY);
	if (fWarp)
	  XWarpPointer(dpy, None, None, 0, 0, 0, 0, FinalX - x, FinalY - y);
      }
      XMapWindow(dpy,w);
    }

  }

  return;
}

void move_window(F_CMD_ARGS)
{
  move_window_doit(eventp,w,tmp_win,context,action,Module,FALSE,FALSE);
}

void animated_move_window(F_CMD_ARGS)
{
  move_window_doit(eventp,w,tmp_win,context,action,Module,TRUE,FALSE);
}

void move_window_to_page(F_CMD_ARGS)
{
  move_window_doit(eventp,w,tmp_win,context,action,Module,FALSE,TRUE);
}

/* This function does the SnapAttraction stuff. If takes x and y coordinates
 * (*px and *py) and returns the snapped values. */
static void DoSnapAttract(
  FvwmWindow *tmp_win, unsigned int Width, unsigned int Height,
  int *px, int *py)
{
  int nyt,nxl,dist,closestLeft,closestRight,closestBottom,closestTop;
  rectangle self, other;
  FvwmWindow *tmp;

  /* resist based on window edges */
  tmp = Scr.FvwmRoot.next;
  closestTop = Scr.SnapAttraction;
  closestBottom = Scr.SnapAttraction;
  closestRight = Scr.SnapAttraction;
  closestLeft = Scr.SnapAttraction;
  nxl = -1;
  nyt = -1;
  self.x = *px;
  self.y = *py;
  self.width = Width;
  self.height = Height;
  while(Scr.SnapAttraction >= 0 && tmp)
  {
    switch (Scr.SnapMode)
    {
    case 1:  /* SameType */
      if( IS_ICONIFIED(tmp) != IS_ICONIFIED(tmp_win))
      {
	tmp = tmp->next;
	continue;
      }
      break;
    case 2:  /* Icons */
      if( !IS_ICONIFIED(tmp) || !IS_ICONIFIED(tmp_win))
      {
	tmp = tmp->next;
	continue;
      }
      break;
    case 3:  /* Windows */
      if( IS_ICONIFIED(tmp) || IS_ICONIFIED(tmp_win))
      {
	tmp = tmp->next;
	continue;
      }
      break;
    case 0:  /* All */
    default:
      /* NOOP */
      break;
    }
    if (tmp_win != tmp && (tmp_win->Desk == tmp->Desk))
    {
      if(IS_ICONIFIED(tmp))
      {
	if(tmp->icon_p_height > 0)
	{
	  other.width = tmp->icon_p_width;
	  other.height = tmp->icon_p_height;
	}
	else
	{
	  other.width = tmp->icon_w_width;
	  other.height = tmp->icon_w_height;
	}
	other.x = tmp->icon_x_loc;
	other.y = tmp->icon_y_loc;
      }
      else
      {
	other.width = tmp->frame_g.width;
	other.height = tmp->frame_g.height;
	other.x = tmp->frame_g.x;
	other.y = tmp->frame_g.y;
      }
      if(!((other.y + (int)other.height) < (*py) ||
	   (other.y) > (*py + (int)self.height) ))
      {
	dist = abs(other.x - (*px + (int)self.width));
	if(dist < closestRight)
	{
	  closestRight = dist;
	  if(((*px + (int)self.width) >= other.x)&&
	     ((*px + (int)self.width) < other.x+Scr.SnapAttraction))
	  {
	    nxl = other.x - (int)self.width;
	  }
	  if(((*px + (int)self.width) >= other.x - Scr.SnapAttraction)&&
	     ((*px + (int)self.width) < other.x))
	  {
	    nxl = other.x - (int)self.width;
	  }
	}
	dist = abs(other.x + (int)other.width - *px);
	if(dist < closestLeft)
	{
	  closestLeft = dist;
	  if((*px <= other.x + (int)other.width)&&
	     (*px > other.x + (int)other.width - Scr.SnapAttraction))
	  {
	    nxl = other.x + (int)other.width;
	  }
	  if((*px <= other.x + (int)other.width + Scr.SnapAttraction)&&
	     (*px > other.x + (int)other.width))
	  {
	    nxl = other.x + (int)other.width;
	  }
	}
      }
      /* ScreenEdges - SJL */
      if(!(Scr.MyDisplayHeight < (*py) ||
	   (*py + (int)self.height) < 0) && (Scr.SnapMode & 8))
      {
	dist = abs(Scr.MyDisplayWidth - (*px + (int)self.width));

	if(dist < closestRight)
	{
	  closestRight = dist;

	  if(((*px + (int)self.width) >= Scr.MyDisplayWidth)&&
	     ((*px + (int)self.width) < Scr.MyDisplayWidth +
	      Scr.SnapAttraction))
	    {
	    nxl = Scr.MyDisplayWidth - (int)self.width;
	  }

	  if(((*px + (int)self.width) >= Scr.MyDisplayWidth -
	      Scr.SnapAttraction)&&
	     ((*px + (int)self.width) < Scr.MyDisplayWidth))
	  {
	    nxl = Scr.MyDisplayWidth - (int)self.width;
	  }
	}

	dist = abs(*px);

	if(dist < closestLeft)
	{
	  closestLeft = dist;

	  if((*px <= 0)&&
	     (*px > - Scr.SnapAttraction))
	  {
	    nxl = 0;
	  }
	  if((*px <= Scr.SnapAttraction)&&
	     (*px > 0))
	  {
	    nxl = 0;
	  }
	}
      }

      if(!((other.x + (int)other.width) < (*px) ||
	   (other.x) > (*px + (int)self.width)))
      {
	dist = abs(other.y - (*py + (int)self.height));
	if(dist < closestBottom)
	{
	  closestBottom = dist;
	  if(((*py + (int)self.height) >= other.y)&&
	     ((*py + (int)self.height) < other.y+Scr.SnapAttraction))
	  {
	    nyt = other.y - (int)self.height;
	  }
	  if(((*py + (int)self.height) >= other.y - Scr.SnapAttraction)&&
	     ((*py + (int)self.height) < other.y))
	  {
	    nyt = other.y - (int)self.height;
	  }
	}
	dist = abs(other.y + (int)other.height - *py);
	if(dist < closestTop)
	{
	  closestTop = dist;
	  if((*py <= other.y + (int)other.height)&&
	     (*py > other.y + (int)other.height - Scr.SnapAttraction))
	  {
	    nyt = other.y + (int)other.height;
	  }
	  if((*py <= other.y + (int)other.height + Scr.SnapAttraction)&&
	     (*py > other.y + (int)other.height))
	  {
	    nyt = other.y + (int)other.height;
	  }
	}
      }
      /* ScreenEdges - SJL */
      if (!(Scr.MyDisplayWidth < (*px) || (*px + (int)self.width) < 0 )
	  && (Scr.SnapMode & 8))
      {
	dist = abs(Scr.MyDisplayHeight - (*py + (int)self.height));

	if(dist < closestBottom)
	{
	  closestBottom = dist;
	  if(((*py + (int)self.height) >= Scr.MyDisplayHeight)&&
	     ((*py + (int)self.height) < Scr.MyDisplayHeight +
	      Scr.SnapAttraction))
	  {
	    nyt = Scr.MyDisplayHeight - (int)self.height;
	  }
	  if(((*py + (int)self.height) >= Scr.MyDisplayHeight -
	      Scr.SnapAttraction)&&
	     ((*py + (int)self.height) < Scr.MyDisplayHeight))
	  {
	    nyt = Scr.MyDisplayHeight - (int)self.height;
	  }
	}

	dist = abs(- *py);

	if(dist < closestTop)
	{
	  closestTop = dist;
	  if((*py <= 0)&&(*py > - Scr.SnapAttraction))
	  {
	    nyt = 0;
	  }
	  if((*py <=  Scr.SnapAttraction)&&(*py > 0))
	  {
	    nyt = 0;
	  }

	}
      }
    }
    tmp = tmp->next;
  }
  /* Snap grid handling */
  if(nxl == -1)
  {
    if(*px != *px / Scr.SnapGridX * Scr.SnapGridX)
    {
      *px = (*px + ((*px >= 0) ? Scr.SnapGridX : -Scr.SnapGridX) / 2) /
	Scr.SnapGridX * Scr.SnapGridX;
    }
  }
  else
  {
    *px = nxl;
  }
  if(nyt == -1)
  {
    if(*py != *py / Scr.SnapGridY * Scr.SnapGridY)
    {
      *py = (*py + ((*py >= 0) ? Scr.SnapGridY : -Scr.SnapGridY) / 2) /
	Scr.SnapGridY * Scr.SnapGridY;
    }
  }
  else
  {
    *py = nyt;
  }
}

/****************************************************************************
 *
 * Move the rubberband around, return with the new window location
 *
 ****************************************************************************/
void moveLoop(FvwmWindow *tmp_win, int XOffset, int YOffset, int Width,
	      int Height, int *FinalX, int *FinalY,Bool opaque_move,
	      Bool AddWindow)
{
  Bool finished = False;
  Bool done;
  int xl,xl2,yt,yt2,delta_x,delta_y,paged;
  unsigned int button_mask = 0;
  FvwmWindow tmp_win_copy;
  int dx = Scr.EdgeScrollX ? Scr.EdgeScrollX : Scr.MyDisplayWidth;
  int dy = Scr.EdgeScrollY ? Scr.EdgeScrollY : Scr.MyDisplayHeight;
  int count;

  /* make a copy of the tmp_win structure for sending to the pager */
  memcpy(&tmp_win_copy, tmp_win, sizeof(FvwmWindow));
  /* prevent flicker when paging */
  SET_WINDOW_BEING_MOVED_OPAQUE(tmp_win, opaque_move);

  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,&xl, &yt,
		&JunkX, &JunkY, &button_mask);
  button_mask &= Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask;
  xl += XOffset;
  yt += YOffset;

  if(((!opaque_move)&&(!Scr.gs.EmulateMWM))||(AddWindow))
    MoveOutline(Scr.Root, xl, yt, Width - 1, Height - 1);

  DisplayPosition(tmp_win,xl,yt,True);

  while (!finished)
  {
    /* wait until there is an interesting event */
    while (!XPending(dpy) ||
	   !XCheckMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
			    KeyPressMask | PointerMotionMask |
			    ButtonMotionMask | ExposureMask, &Event))
    {
      if (HandlePaging(dx, dy, &xl,&yt, &delta_x,&delta_y, False, False, True))
      {
	/* Fake an event to force window reposition */
	xl += XOffset;
	yt += YOffset;
	DoSnapAttract(tmp_win, Width, Height, &xl, &yt);
	Event.type = MotionNotify;
	Event.xmotion.time = lastTimestamp;
	Event.xmotion.x_root = xl - XOffset;
	Event.xmotion.y_root = yt - YOffset;
	break;
      }
    }
    StashEventTime(&Event);

    /* discard any extra motion events before a logical release */
    if (Event.type == MotionNotify)
    {
      XEvent new_event;

      /*** logic borrowed from icewm ***/
      count = Scr.MoveSmoothness;
      /* if count is zero, loop 'infinitely'; this test is not a bug */
      while (--count != 0 && XPending(dpy) > 0 &&
	     XCheckMaskEvent(dpy, ButtonMotionMask | PointerMotionMask |
			     ButtonPressMask | ButtonRelease | KeyPressMask,
			     &new_event))
      {
	if (Event.type != new_event.type)
	{
	  XPutBackEvent(dpy, &new_event);
	  break;
	}
	else
	{
	  Event = new_event;
	}
      }
      /*** end of code borrowed from icewm ***/
      StashEventTime(&Event);

    } /* if (Event.type == MotionNotify) */

    done = FALSE;
    /* Handle a limited number of key press events to allow mouseless
     * operation */
    if (Event.type == KeyPress)
      Keyboard_shortcuts(&Event, tmp_win, ButtonRelease);
    switch(Event.type)
    {
    case KeyPress:
      /* simple code to bag out of move - CKH */
      if (XLookupKeysym(&(Event.xkey),0) == XK_Escape)
      {
	if(!opaque_move)
	  MoveOutline(Scr.Root, 0, 0, 0, 0);
	if (!IS_ICONIFIED(tmp_win))
	{
	  *FinalX = tmp_win->frame_g.x;
	  *FinalY = tmp_win->frame_g.y;
	}
	else
	{
	  *FinalX = tmp_win->icon_x_loc;
	  *FinalY = tmp_win->icon_y_loc;
	}
	finished = TRUE;
      }
      done = TRUE;
      break;
    case ButtonPress:
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
      if (((Event.xbutton.button == 1) && (button_mask & Button1Mask)) ||
	  ((Event.xbutton.button == 2) && (button_mask & Button2Mask)) ||
	  ((Event.xbutton.button == 3) && (button_mask & Button3Mask)) ||
	  ((Event.xbutton.button == 4) && (button_mask & Button4Mask)) ||
	  ((Event.xbutton.button == 5) && (button_mask & Button5Mask)))
      {
	/* No new button was pressed, just a delayed event */
	done = TRUE;
	break;
      }
      if(((Event.xbutton.button == 2)&&(!Scr.gs.EmulateMWM))||
	 ((Event.xbutton.button == 1)&&(Scr.gs.EmulateMWM)&&
	  (Event.xbutton.state & ShiftMask)))
      {
	NeedToResizeToo = True;
	/* Fallthrough to button-release */
      }
      else
      {
	/* Abort the move if
	 *  - the move started with a pressed button and another button
	 *    was pressed during the operation
	 *  - no button was started at the beginning and any button
	 *    except button 1 was pressed. */
	if (button_mask || (Event.xbutton.button != 1))
	{
	  if(!opaque_move)
	    MoveOutline(Scr.Root, 0, 0, 0, 0);
	  if (!IS_ICONIFIED(tmp_win))
	  {
	    *FinalX = tmp_win->frame_g.x;
	    *FinalY = tmp_win->frame_g.y;
	  }
	  else
	  {
	    *FinalX = tmp_win->icon_x_loc;
	    *FinalY = tmp_win->icon_y_loc;
	  }
	  finished = TRUE;
	}
	done = TRUE;
	break;
      }
    case ButtonRelease:
      if(!opaque_move)
	MoveOutline(Scr.Root, 0, 0, 0, 0);
      xl2 = Event.xbutton.x_root + XOffset;
      yt2 = Event.xbutton.y_root + YOffset;
      /* ignore the position of the button release if it was on a
       * different page. */
      if (!(((xl <  0 && xl2 >= 0) || (xl >= 0 && xl2 <  0) ||
	     (yt <  0 && yt2 >= 0) || (yt >= 0 && yt2 <  0)) &&
	    (abs(xl - xl2) > Scr.MyDisplayWidth / 2 ||
	     abs(yt - yt2) > Scr.MyDisplayHeight / 2)))
      {
	xl = xl2;
	yt = yt2;
      }
      DoSnapAttract(tmp_win, Width, Height, &xl, &yt);

      /* Resist moving windows over the edge of the screen! */
      if(((xl + Width) >= Scr.MyDisplayWidth)&&
	 ((xl + Width) < Scr.MyDisplayWidth+Scr.MoveResistance))
	xl = Scr.MyDisplayWidth - Width;
      if((xl <= 0)&&(xl > -Scr.MoveResistance))
	xl = 0;
      if(((yt + Height) >= Scr.MyDisplayHeight)&&
	 ((yt + Height) < Scr.MyDisplayHeight+Scr.MoveResistance))
	yt = Scr.MyDisplayHeight - Height;
      if((yt <= 0)&&(yt > -Scr.MoveResistance))
	yt = 0;

      *FinalX = xl;
      *FinalY = yt;

      done = TRUE;
      finished = TRUE;
      break;

    case MotionNotify:
      xl = Event.xmotion.x_root + XOffset;
      yt = Event.xmotion.y_root + YOffset;

      DoSnapAttract(tmp_win, Width, Height, &xl, &yt);

      /* Resist moving windows over the edge of the screen! */
      if(((xl + Width) >= Scr.MyDisplayWidth)&&
	 ((xl + Width) < Scr.MyDisplayWidth+Scr.MoveResistance))
	xl = Scr.MyDisplayWidth - Width;
      if((xl <= 0)&&(xl > -Scr.MoveResistance))
	xl = 0;
      if(((yt + Height) >= Scr.MyDisplayHeight)&&
	 ((yt + Height) < Scr.MyDisplayHeight+Scr.MoveResistance))
	yt = Scr.MyDisplayHeight - Height;
      if((yt <= 0)&&(yt > -Scr.MoveResistance))
	yt = 0;

      /* check Paging request once and only once after outline redrawn */
      /* redraw after paging if needed - mab */
      paged=0;
      while(paged<=1)
      {
	if(!opaque_move)
	  MoveOutline(Scr.Root, xl, yt, Width - 1, Height - 1);
	else
	{
	  if (IS_ICONIFIED(tmp_win))
	  {
	    tmp_win->icon_x_loc = xl ;
	    tmp_win->icon_xl_loc = xl -
	      (tmp_win->icon_w_width - tmp_win->icon_p_width)/2;
	    tmp_win->icon_y_loc = yt;
	    if(tmp_win->icon_pixmap_w != None)
	      XMoveWindow(dpy, tmp_win->icon_pixmap_w, tmp_win->icon_x_loc,yt);
	    else if (tmp_win->icon_w != None)
	      XMoveWindow(dpy, tmp_win->icon_w,tmp_win->icon_xl_loc,
			  yt+tmp_win->icon_p_height);
	  }
	  else
	  {
	    XMoveWindow(dpy,tmp_win->frame,xl,yt);
	  }
	}
	DisplayPosition(tmp_win,xl,yt,False);

	/* prevent window from lagging behind mouse when paging - mab */
	if(paged==0)
	{
	  xl = Event.xmotion.x_root;
	  yt = Event.xmotion.y_root;
	  HandlePaging(
	    dx, dy, &xl, &yt, &delta_x, &delta_y, False, False, False);
	  xl += XOffset;
	  yt += YOffset;
	  DoSnapAttract(tmp_win, Width, Height, &xl, &yt);
	  if ( (delta_x==0) && (delta_y==0))
	    /* break from while (paged)*/
	    break;
	}
	paged++;
      }  /* end while (paged) */

      done = TRUE;
      break;

    default:
      break;
    } /* switch */
    if(!done)
    {
      DispatchEvent(False);
      if(!opaque_move)
	MoveOutline(Scr.Root, xl, yt, Width - 1, Height - 1);
    }
#ifndef DISABLE_CONFIGURE_NOTIFY_DURING_MOVE
    if (opaque_move && !IS_ICONIFIED(tmp_win) && !IS_SHADED(tmp_win))
    {
      /* send configure notify event for windows that care about their
       * location */
      XEvent client_event;
      client_event.type = ConfigureNotify;
      client_event.xconfigure.display = dpy;
      client_event.xconfigure.event = tmp_win->w;
      client_event.xconfigure.window = tmp_win->w;
      client_event.xconfigure.x = xl + tmp_win->boundary_width;
      client_event.xconfigure.y = yt + tmp_win->title_g.height +
	tmp_win->boundary_width;
      client_event.xconfigure.width = Width - 2 * tmp_win->boundary_width;
      client_event.xconfigure.height = Height -
	2 * tmp_win->boundary_width - tmp_win->title_g.height;
      client_event.xconfigure.border_width = 0;
      /* Real ConfigureNotify events say we're above title window, so... */
      /* what if we don't have a title ????? */
      client_event.xconfigure.above = tmp_win->frame;
      client_event.xconfigure.override_redirect = False;
      XSendEvent(dpy, tmp_win->w, False, StructureNotifyMask,
		 &client_event);
#ifdef FVWM_DEBUG_MSGS
      fvwm_msg(DBG,"SetupFrame","Sent ConfigureNotify (w == %d, h == %d)",
	       client_event.xconfigure.width,
	       client_event.xconfigure.height);
#endif
    }
#endif
    if(opaque_move)
    {
      if (!IS_ICONIFIED(tmp_win))
      {
	tmp_win_copy.frame_g.x = xl;
	tmp_win_copy.frame_g.y = yt;
      }
      /* no point in doing this if server grabbed */
      /* domivogt (20-Aug-1999): Then don't do it. Slows things down quite a
       * bit! */
#if 0
      BroadcastConfig(M_CONFIGURE_WINDOW, &tmp_win_copy);
      FlushOutputQueues();
#endif
    }
  } /* while (!finished) */
  if (!NeedToResizeToo)
    /* Don't wait for buttons to come up when user is placing a new window
     * and wants to resize it. */
    WaitForButtonsUp();
  SET_WINDOW_BEING_MOVED_OPAQUE(tmp_win, 0);

  return;
}

/***********************************************************************
 *
 *  Procedure:
 *      DisplayPosition - display the position in the dimensions window
 *
 *  Inputs:
 *      tmp_win - the current fvwm window
 *      x, y    - position of the window
 *
 ************************************************************************/

static void DisplayPosition(FvwmWindow *tmp_win, int x, int y,int Init)
{
  char str [100];
  int offset;

  (void) sprintf (str, " %+-4d %+-4d ", x, y);
  if(Init)
  {
    XClearWindow(dpy,Scr.SizeWindow);
  }
  else
  {
    /* just clear indside the relief lines to reduce flicker */
    XClearArea(dpy,Scr.SizeWindow,2,2,
	       Scr.SizeStringWidth + SIZE_HINDENT*2 - 3,
	       Scr.StdFont.height + SIZE_VINDENT*2 - 3,False);
  }

  if(Pdepth >= 2)
    RelieveRectangle(dpy,Scr.SizeWindow,0,0,
                     Scr.SizeStringWidth+ SIZE_HINDENT*2 - 1,
                     Scr.StdFont.height + SIZE_VINDENT*2 - 1,
                     Scr.StdReliefGC,
                     Scr.StdShadowGC, 2);
  offset = (Scr.SizeStringWidth + SIZE_HINDENT*2
	    - XTextWidth(Scr.StdFont.font,str,strlen(str)))/2;
#ifdef I18N_MB
  XmbDrawString (dpy, Scr.SizeWindow, Scr.StdFont.fontset, Scr.StdGC,
#else
  XDrawString (dpy, Scr.SizeWindow, Scr.StdGC,
#endif
	       offset,
	       Scr.StdFont.font->ascent + SIZE_VINDENT,
	       str, strlen(str));
}


void SetMoveSmoothness(F_CMD_ARGS)
{
  int val = 0;

  if (GetIntegerArguments(action, NULL, &val, 1) < 1 || val < 0)
    Scr.MoveSmoothness = DEFAULT_MOVE_SMOOTHNESS;
  else
    Scr.MoveSmoothness = val;
}


void SetMoveThreshold(F_CMD_ARGS)
{
  int val = 0;

  if (GetIntegerArguments(action, NULL, &val, 1) < 1 || val < 0)
    Scr.MoveThreshold = DEFAULT_MOVE_THRESHOLD;
  else
    Scr.MoveThreshold = val;
}


void SetOpaque(F_CMD_ARGS)
{
  int val;

  if(GetIntegerArguments(action, NULL, &val, 1) < 1 || val < 0)
    Scr.OpaqueSize = DEFAULT_OPAQUE_MOVE_SIZE;
  else
    Scr.OpaqueSize = val;
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


/* ----------------------------- resizing code ----------------------------- */

/***********************************************************************
 *
 * window resizing borrowed from the "wm" window manager
 *
 ***********************************************************************/

/****************************************************************************
 *
 * Starts a window resize operation
 *
 ****************************************************************************/
void resize_window(F_CMD_ARGS)
{
  Bool finished = FALSE, done = FALSE, abort = FALSE;
  int x,y,delta_x,delta_y,stashed_x,stashed_y;
  Window ResizeWindow;
  Bool fButtonAbort = False;
  Bool fForceRedraw = False;
  int n;
  int values[2];
  int suffix[2];
  unsigned int button_mask = 0;
  rectangle sdrag;
  rectangle sorig;
  rectangle *drag = &sdrag;
  rectangle *orig = &sorig;
  int ymotion=0, xmotion = 0;
  int was_maximized;
  unsigned edge_wrap_x;
  unsigned edge_wrap_y;
  int px;
  int py;
  int i;
  Bool called_from_title = False;

  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_RESIZE, ButtonPress))
    return;

  if (tmp_win == NULL)
    return;

  ResizeWindow = tmp_win->frame;
  XQueryPointer( dpy, ResizeWindow, &JunkRoot, &JunkChild,
		 &JunkX, &JunkY, &px, &py, &button_mask);
  button_mask &= Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask;

  if(check_if_function_allowed(F_RESIZE,tmp_win,True,NULL) == 0)
  {
    XBell(dpy, 0);
    return;
  }

  was_maximized = IS_MAXIMIZED(tmp_win);
  SET_MAXIMIZED(tmp_win, 0);

  /* can't resize icons */
  if(IS_ICONIFIED(tmp_win))
    return;

  /* no suffix = % of screen, 'p' = pixels, 'c' = increment units */
  n = GetSuffixedIntegerArguments(action, NULL, values, 2, "pc", suffix);
  if(n == 2)
  {
    int unit_table[3];

    /* convert the value/suffix pairs to pixels */
    unit_table[0] = Scr.MyDisplayWidth;
    unit_table[1] = 100;
    unit_table[2] = 100 * tmp_win->hints.width_inc;
    drag->width = SuffixToPercentValue(values[0], suffix[0], unit_table);
    if (suffix[0] == 2)
    {
      /* account for base width */
      drag->width += tmp_win->hints.base_width;
    }
    unit_table[0] = Scr.MyDisplayHeight;
    unit_table[2] = 100 * tmp_win->hints.height_inc;
    drag->height = SuffixToPercentValue(values[1], suffix[1], unit_table);
    if (suffix[1] == 2)
    {
      /* account for base height */
      drag->height += tmp_win->hints.base_height;
    }

    drag->width += (2*tmp_win->boundary_width);
    drag->height += (tmp_win->title_g.height + 2*tmp_win->boundary_width);

    /* size will be less or equal to requested */
    ConstrainSize(tmp_win, &drag->width, &drag->height, xmotion, ymotion,
		  False);
    if (IS_SHADED(tmp_win))
    {
      tmp_win->orig_g.width = drag->width;
      tmp_win->orig_g.height = drag->height;
      SetupFrame(tmp_win, tmp_win->frame_g.x, tmp_win->frame_g.y,
		 drag->width, tmp_win->frame_g.height,FALSE,False);
    }
    else
      SetupFrame(tmp_win, tmp_win->frame_g.x, tmp_win->frame_g.y,
		 drag->width, drag->height,FALSE,False);
    SetBorder(tmp_win,True,True,True,None);

    ResizeWindow = None;
    return;
  }

  InstallRootColormap();

  if(!GrabEm(CRS_RESIZE))
  {
    XBell(dpy, 0);
    return;
  }

  MyXGrabServer(dpy);


  /* handle problems with edge-wrapping while resizing */
  edge_wrap_x = Scr.flags.edge_wrap_x;
  edge_wrap_y = Scr.flags.edge_wrap_y;
  Scr.flags.edge_wrap_x = 0;
  Scr.flags.edge_wrap_y = 0;

  if (IS_SHADED(tmp_win))
  {
    drag->x = tmp_win->frame_g.x;
    drag->y = tmp_win->frame_g.y;
    drag->width = tmp_win->frame_g.width;
    if (was_maximized)
      drag->height = tmp_win->maximized_ht;
    else
      drag->height = tmp_win->orig_g.height;
  }
  else
    XGetGeometry(dpy, (Drawable) ResizeWindow, &JunkRoot,
		 &drag->x, &drag->y, (unsigned int *)&drag->width,
		 (unsigned int *)&drag->height, &JunkBW,&JunkDepth);

  orig->x = drag->x;
  orig->y = drag->y;
  orig->width = drag->width;
  orig->height = drag->height;
  ymotion=xmotion=0;

  /* pop up a resize dimensions window */
  XMapRaised(dpy, Scr.SizeWindow);
  DisplaySize(tmp_win, orig->width, orig->height,True,True);

  if((PressedW != Scr.Root)&&(PressedW != None))
  {
    /* Get the current position to determine which border to resize */
    if(PressedW == tmp_win->sides[0])   /* top */
      ymotion = 1;
    else if(PressedW == tmp_win->sides[1])  /* right */
      xmotion = -1;
    else if(PressedW == tmp_win->sides[2])  /* bottom */
      ymotion = -1;
    else if(PressedW == tmp_win->sides[3])  /* left */
      xmotion = 1;
    else if(PressedW == tmp_win->corners[0])  /* upper-left */
    {
      ymotion = 1;
      xmotion = 1;
    }
    else if(PressedW == tmp_win->corners[1])  /* upper-right */
    {
      xmotion = -1;
      ymotion = 1;
    }
    else if(PressedW == tmp_win->corners[2]) /* lower left */
    {
      ymotion = -1;
      xmotion = 1;
    }
    else if(PressedW == tmp_win->corners[3])  /* lower right */
    {
      ymotion = -1;
      xmotion = -1;
    }
  }

  /* begin of code responsible for warping the pointer to the border when
   * starting a resize. */
  if (tmp_win->title_w != None && PressedW == tmp_win->title_w)
  {
    /* title was pressed to thart the resize */
    called_from_title = True;
  }
  else
  {
    for (i = 5; i--; )
    {
      /* see if the title button was pressed to that the resize */
      if ((tmp_win->left_w[i] != None && PressedW == tmp_win->left_w[i]) ||
	  (tmp_win->right_w[i] != None && PressedW == tmp_win->right_w[i]))
      {
	/* yes */
	called_from_title = True;
      }
    }
  }
  /* don't warp if the resize was triggered by a press somwhere on the title
   * bar */
  if(PressedW != Scr.Root && xmotion == 0 && ymotion == 0 &&
     !called_from_title)
  {
    int dx = orig->width - px;
    int dy = orig->height - py;
    int wx = -1;
    int wy = -1;
    int tx;
    int ty;

    /* Now find the place to warp to. We Simply use the sectors drawn when we
     * start resizing the window. */
    tx = orig->width / 3 - 1;
    ty = orig->height / 3 - 1;
    if (px >= 0 && dx >= 0 && py >= 0 && dy >= 0)
    {
      if (px < tx)
      {
	if (py < ty)
	{
	  xmotion = 1;
	  ymotion = 1;
	  wx = 0;
	  wy = 0;
	}
	else if (dy < ty)
	{
	  xmotion = 1;
	  ymotion = -1;
	  wx = 0;
	  wy = orig->height -1;
	}
	else
	{
	  xmotion = 1;
	  wx = 0;
	  wy = orig->height/2;
	}
      }
      else if (dx < tx)
      {
	if (py < ty)
	{
	  xmotion = -1;
	  ymotion = 1;
	  wx = orig->width - 1;
	  wy = 0;
	}
	else if (dy < ty)
	{
	  xmotion = -1;
	  ymotion = -1;
	  wx = orig->width - 1;
	  wy = orig->height -1;
	}
	else
	{
	  xmotion = -1;
	  wx = orig->width - 1;
	  wy = orig->height/2;
	}
      }
      else
      {
	if (py < ty)
	{
	  ymotion = 1;
	  wx = orig->width/2;
	  wy = 0;
	}
	else if (dy < ty)
	{
	  ymotion = -1;
	  wx = orig->width/2;
	  wy = orig->height -1;
	}
      }
    }

    if (wx != -1)
    {
      /* now warp the pointer to the border */
      XWarpPointer(dpy, None, ResizeWindow, 0, 0, 1, 1, wx, wy);
      XFlush(dpy);
    }
  }
  /* end of code responsible for warping the pointer to the border when
   * starting a resize. */

  /* draw the rubber-band window */
  MoveOutline(Scr.Root, drag->x, drag->y, drag->width - 1, drag->height - 1);
  /* kick off resizing without requiring any motion if invoked with a key
   * press */
  if (eventp->type == KeyPress)
  {
    XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		  &stashed_x,&stashed_y,&JunkX, &JunkY, &JunkMask);
    DoResize(stashed_x, stashed_y, tmp_win, drag, orig, &xmotion, &ymotion);
  }
  else
    stashed_x = stashed_y = -1;

  /* loop to resize */
  while(!finished)
  {
    /* block until there is an interesting event */
    while (!XCheckMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
			    KeyPressMask | PointerMotionMask |
			    ButtonMotionMask | ExposureMask, &Event))
    {
      if (HandlePaging(Scr.EdgeScrollX, Scr.EdgeScrollY, &x, &y,
		       &delta_x, &delta_y, False, False, True))
      {
	/* Fake an event to force window reposition */
	Event.type = MotionNotify;
	Event.xmotion.time = lastTimestamp;
	fForceRedraw = True;
	break;
      }
    }
    StashEventTime(&Event);

    if (Event.type == MotionNotify)
      /* discard any extra motion events before a release */
      while(XCheckMaskEvent(dpy, ButtonMotionMask | PointerMotionMask |
			    ButtonReleaseMask | ButtonPressMask, &Event))
      {
	StashEventTime(&Event);
	if (Event.type == ButtonRelease || Event.type == ButtonPress)
	  break;
      }

    done = FALSE;
    /* Handle a limited number of key press events to allow mouseless
     * operation */
    if(Event.type == KeyPress)
      Keyboard_shortcuts(&Event, tmp_win, ButtonRelease);
    switch(Event.type)
    {
    case ButtonPress:
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
      done = TRUE;
      if (((Event.xbutton.button == 1) && (button_mask & Button1Mask)) ||
	  ((Event.xbutton.button == 2) && (button_mask & Button2Mask)) ||
	  ((Event.xbutton.button == 3) && (button_mask & Button3Mask)) ||
	  ((Event.xbutton.button == 4) && (button_mask & Button4Mask)) ||
	  ((Event.xbutton.button == 5) && (button_mask & Button5Mask)))
      {
	/* No new button was pressed, just a delayed event */
	break;
      }
      /* Abort the resize if
       *  - the move started with a pressed button and another button
       *    was pressed during the operation
       *  - no button was started at the beginning and any button
       *    except button 1 was pressed. */
      if (button_mask || (Event.xbutton.button != 1))
	fButtonAbort = TRUE;
    case KeyPress:
      /* simple code to bag out of move - CKH */
      if (XLookupKeysym(&(Event.xkey),0) == XK_Escape || fButtonAbort)
      {
	abort = TRUE;
	finished = TRUE;
	/* return pointer if aborted resize was invoked with key */
	if (stashed_x >= 0)
	  XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, stashed_x,
		       stashed_y);
      }
      done = TRUE;
      break;

    case ButtonRelease:
      finished = TRUE;
      done = TRUE;
      break;

    case MotionNotify:
      if (!fForceRedraw)
      {
	x = Event.xmotion.x_root;
	y = Event.xmotion.y_root;
	/* resize before paging request to prevent resize from lagging
	 * mouse - mab */
	DoResize(x, y, tmp_win, drag, orig, &xmotion, &ymotion);
	/* need to move the viewport */
	HandlePaging(Scr.EdgeScrollX, Scr.EdgeScrollY, &x, &y,
		     &delta_x, &delta_y, False, False, False);
      }
      /* redraw outline if we paged - mab */
      if ( (delta_x != 0) || (delta_y != 0) )
      {
	orig->x -= delta_x;
	orig->y -= delta_y;
	drag->x -= delta_x;
	drag->y -= delta_y;

	DoResize(x, y, tmp_win, drag, orig, &xmotion, &ymotion);
      }
      fForceRedraw = False;
      done = TRUE;
    default:
      break;
    }
    if(!done)
    {
      DispatchEvent(False);
      MoveOutline(Scr.Root, drag->x, drag->y,
		  drag->width - 1,
		  drag->height - 1);
    }
  }

  /* erase the rubber-band */
  MoveOutline(Scr.Root, 0, 0, 0, 0);

  /* pop down the size window */
  XUnmapWindow(dpy, Scr.SizeWindow);

  if(!abort)
  {
    /* size will be >= to requested */
    ConstrainSize(tmp_win, &drag->width, &drag->height, xmotion, ymotion,
		  True);
    if (IS_SHADED(tmp_win))
    {
      SetupFrame(tmp_win, drag->x, drag->y,
		 drag->width, tmp_win->frame_g.height, FALSE, False);
      tmp_win->orig_g.height = drag->height;
    }
    else
      SetupFrame(tmp_win, drag->x, drag->y, drag->width, drag->height,
		 FALSE, False);
  }
  UninstallRootColormap();
  ResizeWindow = None;
  MyXUngrabServer(dpy);
  UngrabEm();
  xmotion = 0;
  ymotion = 0;
  WaitForButtonsUp();

  Scr.flags.edge_wrap_x = edge_wrap_x;
  Scr.flags.edge_wrap_y = edge_wrap_y;
  return;
}



/***********************************************************************
 *
 *  Procedure:
 *      DoResize - move the rubberband around.  This is called for
 *                 each motion event when we are resizing
 *
 *  Inputs:
 *      x_root   - the X corrdinate in the root window
 *      y_root   - the Y corrdinate in the root window
 *      tmp_win  - the current fvwm window
 *      drag     - resize internal structure
 *      orig     - resize internal structure
 *      xmotionp - pointer to xmotion in resize_window
 *      ymotionp - pointer to ymotion in resize_window
 *
 ************************************************************************/
static void DoResize(int x_root, int y_root, FvwmWindow *tmp_win,
		     rectangle *drag, rectangle *orig, int *xmotionp,
		     int *ymotionp)
{
  int action=0;

  if ((y_root <= orig->y) ||
      ((*ymotionp == 1)&&(y_root < orig->y+orig->height-1)))
  {
    drag->y = y_root;
    drag->height = orig->y + orig->height - y_root;
    action = 1;
    *ymotionp = 1;
  }
  else if ((y_root >= orig->y + orig->height - 1)||
	   ((*ymotionp == -1)&&(y_root > orig->y)))
  {
    drag->y = orig->y;
    drag->height = 1 + y_root - drag->y;
    action = 1;
    *ymotionp = -1;
  }

  if ((x_root <= orig->x)||
      ((*xmotionp == 1)&&(x_root < orig->x + orig->width - 1)))
  {
    drag->x = x_root;
    drag->width = orig->x + orig->width - x_root;
    action = 1;
    *xmotionp = 1;
  }
  if ((x_root >= orig->x + orig->width - 1)||
    ((*xmotionp == -1)&&(x_root > orig->x)))
  {
    drag->x = orig->x;
    drag->width = 1 + x_root - orig->x;
    action = 1;
    *xmotionp = -1;
  }

  if (action)
  {
    /* round up to nearest OK size to keep pointer inside rubberband */
    ConstrainSize(tmp_win, &drag->width, &drag->height, *xmotionp, *ymotionp,
		  True);
    if (*xmotionp == 1)
      drag->x = orig->x + orig->width - drag->width;
    if (*ymotionp == 1)
      drag->y = orig->y + orig->height - drag->height;

    MoveOutline(Scr.Root, drag->x, drag->y,
		drag->width - 1,
		drag->height - 1);
  }
  DisplaySize(tmp_win, drag->width, drag->height,False,False);
}



/***********************************************************************
 *
 *  Procedure:
 *      DisplaySize - display the size in the dimensions window
 *
 *  Inputs:
 *      tmp_win - the current fvwm window
 *      width   - the width of the rubber band
 *      height  - the height of the rubber band
 *
 ***********************************************************************/
static void DisplaySize(FvwmWindow *tmp_win, int width, int height, Bool Init,
			Bool resetLast)
{
  char str[100];
  int dwidth,dheight,offset;
  static int last_width = 0;
  static int last_height = 0;

  if (resetLast)
  {
    last_width = 0;
    last_height = 0;
  }
  if (last_width == width && last_height == height)
    return;

  last_width = width;
  last_height = height;

  dheight = height - tmp_win->title_g.height - 2*tmp_win->boundary_width;
  dwidth = width - 2*tmp_win->boundary_width;

  dwidth -= tmp_win->hints.base_width;
  dheight -= tmp_win->hints.base_height;
  dwidth /= tmp_win->hints.width_inc;
  dheight /= tmp_win->hints.height_inc;

  (void) sprintf (str, " %4d x %-4d ", dwidth, dheight);
  if(Init)
  {
    XClearWindow(dpy,Scr.SizeWindow);
  }
  else
  {
    /* just clear indside the relief lines to reduce flicker */
    XClearArea(dpy,Scr.SizeWindow,2,2,
	       Scr.SizeStringWidth + SIZE_HINDENT*2 - 3,
	       Scr.StdFont.height + SIZE_VINDENT*2 - 3,False);
  }

  if(Pdepth >= 2)
    RelieveRectangle(dpy,Scr.SizeWindow,0,0,
                     Scr.SizeStringWidth+ SIZE_HINDENT*2 - 1,
                     Scr.StdFont.height + SIZE_VINDENT*2 - 1,
                     Scr.StdReliefGC,
                     Scr.StdShadowGC, 2);
  offset = (Scr.SizeStringWidth + SIZE_HINDENT*2
    - XTextWidth(Scr.StdFont.font,str,strlen(str)))/2;
#ifdef I18N_MB
  XmbDrawString (dpy, Scr.SizeWindow, Scr.StdFont.fontset, Scr.StdGC,
		 offset, Scr.StdFont.font->ascent + SIZE_VINDENT, str, 13);
#else
  XDrawString (dpy, Scr.SizeWindow, Scr.StdGC,
	       offset, Scr.StdFont.font->ascent + SIZE_VINDENT, str, 13);
#endif
}

/***********************************************************************
 *
 *  Procedure:
 *      ConstrainSize - adjust the given width and height to account for the
 *              constraints imposed by size hints
 *
 *      The general algorithm, especially the aspect ratio stuff, is
 *      borrowed from uwm's CheckConsistency routine.
 *
 ***********************************************************************/

void ConstrainSize(
  FvwmWindow *tmp_win, unsigned int *widthp, unsigned int *heightp,
  int xmotion, int ymotion, Bool roundUp)
{
#define MAKEMULT(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
  int minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
  int baseWidth, baseHeight;
  int dwidth = *widthp, dheight = *heightp;
  int roundUpX = 0;
  int roundUpY = 0;

  dwidth -= 2 *tmp_win->boundary_width;
  dheight -= (tmp_win->title_g.height + 2 * tmp_win->boundary_width);

  minWidth = tmp_win->hints.min_width;
  minHeight = tmp_win->hints.min_height;

  maxWidth = tmp_win->hints.max_width;
  maxHeight =  tmp_win->hints.max_height;

  if (maxWidth > tmp_win->max_window_width - 2 * tmp_win->boundary_width)
  {
    maxWidth = tmp_win->max_window_width - 2 * tmp_win->boundary_width;
  }
  if (maxHeight > tmp_win->max_window_height - 2*tmp_win->boundary_width -
      tmp_win->title_g.height)
  {
    maxHeight =
      tmp_win->max_window_height - 2*tmp_win->boundary_width -
      tmp_win->title_g.height;
  }

  baseWidth = tmp_win->hints.base_width;
  baseHeight = tmp_win->hints.base_height;

  xinc = tmp_win->hints.width_inc;
  yinc = tmp_win->hints.height_inc;

  /*
   * First, clamp to min and max values
   */
  if (dwidth < minWidth)
    dwidth = minWidth;
  if (dheight < minHeight)
    dheight = minHeight;

  if (dwidth > maxWidth)
    dwidth = maxWidth;
  if (dheight > maxHeight)
    dheight = maxHeight;


  /*
   * Second, round to base + N * inc (up or down depending on resize type)
   * if rounding up store amount
   */
  if (!roundUp)
  {
    dwidth = (((dwidth - baseWidth) / xinc) * xinc) + baseWidth;
    dheight = (((dheight - baseHeight) / yinc) * yinc) + baseHeight;
  }
  else
  {
    roundUpX = dwidth;
    roundUpY = dheight;
    dwidth = (((dwidth - baseWidth + xinc - 1) / xinc) * xinc) + baseWidth;
    dheight = (((dheight - baseHeight + yinc - 1) / yinc) * yinc) +
      baseHeight;
    roundUpX = dwidth - roundUpX;
    roundUpY = dheight - roundUpY;
  }

  /*
   * Step 2a: check we didn't move the edge off screen in interactive moves
   */
  if (roundUp && Event.type == MotionNotify)
  {
    if (xmotion > 0 && Event.xmotion.x_root < roundUpX)
      dwidth -= xinc;
    else if (xmotion < 0 &&
	     Event.xmotion.x_root >= Scr.MyDisplayWidth - roundUpX)
      dwidth -= xinc;
    if (ymotion > 0 && Event.xmotion.y_root < roundUpY)
      dheight -= yinc;
    else if (ymotion < 0 &&
	     Event.xmotion.y_root >= Scr.MyDisplayHeight - roundUpY)
      dheight -= yinc;
  }

  /*
   * Step 2b: Check that we didn't violate min and max.
   */
  if (dwidth < minWidth)
    dwidth += xinc;
  if (dheight < minHeight)
    dheight += yinc;
  if (dwidth > maxWidth)
    dwidth -= xinc;
  if (dheight > maxHeight)
    dheight -= yinc;

  /*
   * Third, adjust for aspect ratio
   */
#define maxAspectX tmp_win->hints.max_aspect.x
#define maxAspectY tmp_win->hints.max_aspect.y
#define minAspectX tmp_win->hints.min_aspect.x
#define minAspectY tmp_win->hints.min_aspect.y
  /*
   * The math looks like this:
   *
   * minAspectX    dwidth     maxAspectX
   * ---------- <= ------- <= ----------
   * minAspectY    dheight    maxAspectY
   *
   * If that is multiplied out, then the width and height are
   * invalid in the following situations:
   *
   * minAspectX * dheight > minAspectY * dwidth
   * maxAspectX * dheight < maxAspectY * dwidth
   *
   */

  if (tmp_win->hints.flags & PAspect)
  {

    if (tmp_win->hints.flags & PBaseSize)
    {
      /*
	ICCCM 2 demands that aspect ratio should apply
	to width - base_width. To prevent funny results,
	we reset PBaseSize in GetWindowSizeHints, if
	base is not smaller than min.
      */
      dwidth -= baseWidth;
      maxWidth -= baseWidth;
      minWidth -= baseWidth;
      dheight -= baseHeight;
      maxHeight -= baseHeight;
      minHeight -= baseHeight;
    }

    if ((minAspectX * dheight > minAspectY * dwidth)&&(xmotion == 0))
    {
      /* Change width to match */
      delta = MAKEMULT(minAspectX * dheight / minAspectY - dwidth,
		       xinc);
      if (dwidth + delta <= maxWidth)
	dwidth += delta;
    }
    if (minAspectX * dheight > minAspectY * dwidth)
    {
      delta = MAKEMULT(dheight - dwidth*minAspectY/minAspectX,
		       yinc);
      if (dheight - delta >= minHeight)
	dheight -= delta;
      else
      {
	delta = MAKEMULT(minAspectX*dheight / minAspectY - dwidth,
			 xinc);
	if (dwidth + delta <= maxWidth)
	  dwidth += delta;
      }
    }

    if ((maxAspectX * dheight < maxAspectY * dwidth)&&(ymotion == 0))
    {
      delta = MAKEMULT(dwidth * maxAspectY / maxAspectX - dheight,
		       yinc);
      if (dheight + delta <= maxHeight)
	dheight += delta;
    }
    if ((maxAspectX * dheight < maxAspectY * dwidth))
    {
      delta = MAKEMULT(dwidth - maxAspectX*dheight/maxAspectY,
		       xinc);
      if (dwidth - delta >= minWidth)
	dwidth -= delta;
      else
      {
	delta = MAKEMULT(dwidth * maxAspectY / maxAspectX - dheight, yinc);
	if (dheight + delta <= maxHeight)
	  dheight += delta;
      }
    }

    if (tmp_win->hints.flags & PBaseSize)
    {
      dwidth += baseWidth;
      dheight += baseHeight;
    }
  }


  /*
   * Fourth, account for border width and title height
   */
  *widthp = dwidth + 2*tmp_win->boundary_width;
  *heightp = dheight + tmp_win->title_g.height + 2*tmp_win->boundary_width;

  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	MoveOutline - move a window outline
 *
 *  Inputs:
 *	root	    - the window we are outlining
 *	x	    - upper left x coordinate
 *	y	    - upper left y coordinate
 *	width	    - the width of the rectangle
 *	height	    - the height of the rectangle
 *
 ***********************************************************************/
void MoveOutline(Window root, int x, int  y, int  width, int height)
{
  static int lastx = 0;
  static int lasty = 0;
  static int lastWidth = 0;
  static int lastHeight = 0;
  int interleave = 0;
  int offset;
  XRectangle rects[10];

  if (x == lastx && y == lasty && width == lastWidth && height == lastHeight)
    return;

  /* figure out the ordering */
  if (width || height)
    interleave += 1;
  if (lastWidth || lastHeight)
    interleave += 1;
  offset = interleave >> 1;

  /* place the resize rectangle into the array of rectangles */
  /* interleave them for best visual look */
  /* draw the new one, if any */
  if (width || height)
  {
    int i;
    for (i=0; i < 4; i++)
    {
      rects[i * interleave].x = x + i;
      rects[i * interleave].y = y + i;
      rects[i * interleave].width = width - (i << 1);
      rects[i * interleave].height = height - (i << 1);
    }
    rects[3 * interleave].y = y+3 + (height-6)/3;
    rects[3 * interleave].height = (height-6)/3;
    rects[4 * interleave].x = x+3 + (width-6)/3;
    rects[4 * interleave].y = y+3;
    rects[4 * interleave].width = (width-6)/3;
    rects[4 * interleave].height = (height-6);
  }

  /* undraw the old one, if any */
  if (lastWidth || lastHeight)
  {
    int i;
    for (i=0; i < 4; i++)
    {
      rects[i * interleave + offset].x = lastx + i;
      rects[i * interleave + offset].y = lasty + i;
      rects[i * interleave + offset].width = lastWidth - (i << 1);
      rects[i * interleave + offset].height = lastHeight - (i << 1);
    }
    rects[3 * interleave + offset].y = lasty+3 + (lastHeight-6)/3;
    rects[3 * interleave + offset].height = (lastHeight-6)/3;
    rects[4 * interleave + offset].x = lastx+3 + (lastWidth-6)/3;
    rects[4 * interleave + offset].y = lasty+3;
    rects[4 * interleave + offset].width = (lastWidth-6)/3;
    rects[4 * interleave + offset].height = (lastHeight-6);
  }

  XDrawRectangles(dpy,Scr.Root,Scr.DrawGC,rects,interleave * 5);

  lastx = x;
  lasty = y;
  lastWidth = width;
  lastHeight = height;

  return;
}


/* ----------------------------- maximizing code --------------------------- */

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

  for (cwin = Scr.FvwmRoot.next; cwin; cwin = cwin->next)
  {
    if (IS_STICKY(cwin) || (IS_ICONIFIED(cwin) && IS_ICON_STICKY(cwin)))
      is_sticky = True;
    else
      is_sticky = False;
    if (cwin == win || (cwin->Desk != win->Desk && !is_sticky))
      continue;
    if (IS_ICONIFIED(cwin))
    {
      if(cwin->icon_w == None || IS_ICON_UNMAPPED(cwin))
	continue;
      x21 = cwin->icon_x_loc;
      y21 = cwin->icon_y_loc;
      x22 = x21 + cwin->icon_p_width;
      y22 = y21 + cwin->icon_p_height + cwin->icon_w_height;
    }
    else
    {
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
    if (!((x22 <= x11) || (x21 >= x12)))
    {
      if ((y22 <= y11) && (y22 >= new_y1))
      {
	new_y1 = y22;
      }
      else if ((y12 <= y21) && (new_y2 >= y21))
      {
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

  for (cwin = Scr.FvwmRoot.next; cwin; cwin = cwin->next)
  {
    if (IS_STICKY(cwin) || (IS_ICONIFIED(cwin) && IS_ICON_STICKY(cwin)))
      is_sticky = True;
    else
      is_sticky = False;
    if (cwin == win || (cwin->Desk != win->Desk && !is_sticky))
      continue;
    if (IS_ICONIFIED(cwin))
    {
      if(cwin->icon_w == None || IS_ICON_UNMAPPED(cwin))
	continue;
      x21 = cwin->icon_x_loc;
      y21 = cwin->icon_y_loc;
      x22 = x21 + cwin->icon_p_width;
      y22 = y21 + cwin->icon_p_height + cwin->icon_w_height;
    }
    else
    {
      x21 = cwin->frame_g.x;
      y21 = cwin->frame_g.y;
      x22 = x21 + cwin->frame_g.width;
      y22 = y21 + cwin->frame_g.height;
    }
    if (is_sticky)
    {
      move_sticky_window_to_same_page(
	&new_x1, &new_x2, &y21, &y22, x11, x12, y11, y12);
    }

    /* Are they in the same Y space? */
    if (!((y22 <= y11) || (y21 >= y12)))
    {
      if ((x22 <= x11) && (x22 >= new_x1))
      {
	new_x1 = x22;
      }
      else if ((x12 <= x21) && (new_x2 >= x21))
      {
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
      new_height = Scr.MyDisplayHeight - 1;
      new_width = Scr.MyDisplayWidth - 1;
    }
    SET_MAXIMIZED(tmp_win, 1);
    ConstrainSize(tmp_win, &new_width, &new_height, 0, 0, False);
    tmp_win->maximized_ht = new_height;
    if (IS_SHADED(tmp_win))
      new_height = tmp_win->frame_g.height;
    SetupFrame(tmp_win,new_x,new_y,new_width,new_height,TRUE,False);
    SetBorder(tmp_win,Scr.Hilite == tmp_win,True,True,None);
  }
}
