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
#include "placement.h"
#include "gnome.h"

/* ----- move globals ----- */
extern XEvent Event;

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

static void DoResize(
  int x_root, int y_root, FvwmWindow *tmp_win, rectangle *drag,
  rectangle *orig, int *xmotionp, int *ymotionp, Bool do_resize_opaque);
static void DisplaySize(FvwmWindow *, int, int, Bool, Bool);
/* ----- end of resize globals ----- */

/* The vars are named for the x-direction, but this is used for both x and y */
static int GetOnePositionArgument(
  char *s1,int x,int w,int *pFinalX,float factor, int max, Bool is_x)
{
  int val;
  int cch = strlen(s1);
  Bool add_pointer_position = False;

  if (cch == 0)
    return 0;
  if (s1[cch-1] == 'p')
  {
    factor = 1;  /* Use pixels, so don't multiply by factor */
    s1[cch-1] = '\0';
  }
  if (*s1 == 'm')
  {
    add_pointer_position = True;
    s1++;
    /* 'mw' is not allowed */
    if (*s1 == 'w')
      return 0;
  }
  if (strcmp(s1,"w") == 0)
  {
    *pFinalX = x;
  }
  else if (sscanf(s1,"w-%d",&val) == 1)
  {
    *pFinalX = x-(val*factor);
  }
  else if (sscanf(s1,"w+%d",&val) == 1 || sscanf(s1,"w%d",&val) == 1 )
  {
    *pFinalX = x+(val*factor);
  }
  else if (sscanf(s1,"-%d",&val) == 1)
  {
    *pFinalX = max-w - val*factor;
  }
  else if (sscanf(s1,"%d",&val) == 1)
  {
    *pFinalX = val*factor;
  }
  else
  {
    return 0;
  }
  if (add_pointer_position)
  {
    int x = 0;
    int y = 0;

    XQueryPointer(
      dpy, Scr.Root, &JunkRoot, &JunkChild, &JunkX, &JunkY, &x, &y, &JunkMask);
    *pFinalX += (is_x) ? x : y;
  }

  return 1;
}

/* GetMoveArguments is used for Move & AnimatedMove
 * It lets you specify in all the following ways
 *   20  30          Absolute percent position, from left edge and top
 *  -50  50          Absolute percent position, from right edge and top
 *   10p 5p          Absolute pixel position
 *   10p -0p         Absolute pixel position, from bottom
 *  w+5  w-10p       Relative position, right 5%, up ten pixels
 *  w+5  w-10p       Pointer relative position, right 5%, up ten pixels
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
    if (GetOnePositionArgument(s1, x, w, pFinalX, (float)scrWidth/100,
			       scrWidth, True) &&
        GetOnePositionArgument(s2, y, h, pFinalY, (float)scrHeight/100,
			       scrHeight, False))
      retval = 2;
    else
      *fWarp = False; /* make sure warping is off for interactive moves */
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
  Bool do_move_opaque = False;

  w = *win;

  if (Scr.bo.InstallRootCmap)
    InstallRootColormap();
  else
    InstallFvwmColormap();
  /* warp the pointer to the cursor position from before menu appeared*/
  /* domivogt (17-May-1999): an XFlush should not hurt anyway, so do it
   * unconditionally to remove the external */
  XFlush(dpy);

  /* Although a move is usually done with a button depressed we have to check
   * for ButtonRelease too since the event may be faked. */
  GetLocationFromEventOrQuery(dpy, Scr.Root, &Event, &DragX, &DragY);

  if(!GrabEm(CRS_MOVE, GRAB_NORMAL))
  {
    XBell(dpy, 0);
    return;
  }

  XGetGeometry(dpy, w, &JunkRoot, &origDragX, &origDragY,
	       (unsigned int *)&DragWidth, (unsigned int *)&DragHeight,
	       &JunkBW,  &JunkDepth);

  if(DragWidth*DragHeight <
     (Scr.OpaqueSize*Scr.MyDisplayWidth*Scr.MyDisplayHeight)/100)
    do_move_opaque = True;
  else
    MyXGrabServer(dpy);

  if((!do_move_opaque)&&(IS_ICONIFIED(tmp_win)))
    XUnmapWindow(dpy,w);

  XOffset = origDragX - DragX;
  YOffset = origDragY - DragY;
  if (!Scr.gs.do_hide_position_window)
    XMapRaised(dpy,Scr.SizeWindow);
  moveLoop(tmp_win, XOffset,YOffset,DragWidth,DragHeight, FinalX,FinalY,
	   do_move_opaque,False);
  if (Scr.bo.InstallRootCmap)
    UninstallRootColormap();
  else
    UninstallFvwmColormap();

  if(!do_move_opaque)
    MyXUngrabServer(dpy);
  UngrabEm(GRAB_NORMAL);
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
    if (fWarpPointerToo == True)
    {
      XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &JunkX,&JunkY,&pointerX,&pointerY,&JunkMask);
      pointerX += currentX - lastX;
      pointerY += currentY - lastY;
      XWarpPointer(dpy,None,Scr.Root,0,0,0,0,
		   pointerX,pointerY);
    }
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
      client_event.xconfigure.y = currentY + tmp_win->boundary_width +
	((HAS_BOTTOM_TITLE(tmp_win)) ? 0 : tmp_win->title_g.height);
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
    XFlush(dpy);
    if (tmp_win)
    {
      tmp_win->frame_g.x = currentX;
      tmp_win->frame_g.y = currentY;
      update_absolute_geometry(tmp_win);
      maximize_adjust_offset(tmp_win);
      BroadcastConfig(M_CONFIGURE_WINDOW, tmp_win);
      FlushOutputQueues();
    }

    usleep(cmsDelay * 1000); /* usleep takes microseconds */
    /* this didn't work for me -- maybe no longer necessary since
     * we warn the user when they use > .5 seconds as a between-frame delay
     * time.
     *
     * domivogt (28-apr-1999): That is because the keyboard was not grabbed.
     * works nicely now.
     */
    if (XCheckMaskEvent(dpy, ButtonPressMask|ButtonReleaseMask|KeyPressMask,
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
void move_window_doit(F_CMD_ARGS, Bool do_animate, Bool do_move_to_page)
{
  int FinalX, FinalY;
  int n;
  int x,y;
  unsigned int width, height;
  int page_x, page_y;
  Bool fWarp = False;
  int dx;
  int dy;

  if (DeferExecution(eventp,&w,&tmp_win,&context,
		     (do_move_to_page) ? CRS_SELECT : CRS_MOVE, ButtonPress))
    return;

  if (tmp_win == NULL)
    return;

#if 0
fprintf(stderr,"move window '%s'\n", tmp_win->name);
#endif
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
  if (do_move_to_page)
  {
    do_animate = False;
    FinalX = x % Scr.MyDisplayWidth;
    FinalY = y % Scr.MyDisplayHeight;
    if(FinalX < 0)
      FinalX += Scr.MyDisplayWidth;
    if(FinalY < 0)
      FinalY += Scr.MyDisplayHeight;
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

  dx = FinalX - tmp_win->frame_g.x;
  dy = FinalY - tmp_win->frame_g.y;
  if (w == tmp_win->frame)
  {
    if (do_animate)
    {
      AnimatedMoveFvwmWindow(tmp_win,w,-1,-1,FinalX,FinalY,fWarp,-1,NULL);
    }
    SetupFrame(tmp_win, FinalX, FinalY,
	       tmp_win->frame_g.width, tmp_win->frame_g.height, True);
    if (fWarp & !do_animate)
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
    if (do_animate)
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
      if (do_animate)
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
  if (IS_MAXIMIZED(tmp_win))
  {
    tmp_win->max_g.x += dx;
    tmp_win->max_g.y += dy;
    update_absolute_geometry(tmp_win);
    maximize_adjust_offset(tmp_win);
  }
  else
  {
    tmp_win->normal_g.x += dx;
    tmp_win->normal_g.y += dy;
    update_absolute_geometry(tmp_win);
  }

  return;
}

void move_window(F_CMD_ARGS)
{
  move_window_doit(eventp,w,tmp_win,context,action,Module,False,False);
}

void animated_move_window(F_CMD_ARGS)
{
  move_window_doit(eventp,w,tmp_win,context,action,Module,True,False);
}

void move_window_to_page(F_CMD_ARGS)
{
  move_window_doit(eventp,w,tmp_win,context,action,Module,False,True);
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
 * Returns True if the window has to be resized after the move.
 *
 ****************************************************************************/
Bool moveLoop(FvwmWindow *tmp_win, int XOffset, int YOffset, int Width,
	      int Height, int *FinalX, int *FinalY,Bool do_move_opaque,
	      Bool AddWindow)
{
  extern Window bad_window;
  Bool finished = False;
  Bool done;
  Bool aborted = False;
  int xl,xl2,yt,yt2,delta_x,delta_y,paged;
  unsigned int button_mask = 0;
  FvwmWindow tmp_win_copy;
  int dx = Scr.EdgeScrollX ? Scr.EdgeScrollX : Scr.MyDisplayWidth;
  int dy = Scr.EdgeScrollY ? Scr.EdgeScrollY : Scr.MyDisplayHeight;
  int count;
  int vx = Scr.Vx;
  int vy = Scr.Vy;
  int xl_orig;
  int yt_orig;
  int cnx = 0;
  int cny = 0;
  Bool sent_cn = False;
  Bool do_resize_too = False;
  int x_bak;
  int y_bak;
  Window move_w = None;

  bad_window = None;
  if (IS_ICONIFIED(tmp_win))
  {
    if(tmp_win->icon_pixmap_w != None)
      move_w = tmp_win->icon_pixmap_w;
    else if (tmp_win->icon_w != None)
      move_w = tmp_win->icon_w;
  }
  else
  {
    move_w = tmp_win->frame;
  }
  XGetGeometry(dpy, move_w, &JunkRoot, &x_bak, &y_bak, &JunkWidth, &JunkHeight,
	       &JunkBW,&JunkDepth);

  /* make a copy of the tmp_win structure for sending to the pager */
  memcpy(&tmp_win_copy, tmp_win, sizeof(FvwmWindow));
  /* prevent flicker when paging */
  SET_WINDOW_BEING_MOVED_OPAQUE(tmp_win, do_move_opaque);

  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,&xl, &yt,
		&JunkX, &JunkY, &button_mask);
  button_mask &= Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask;
  xl += XOffset;
  yt += YOffset;
  xl_orig = xl;
  yt_orig = yt;

  if(((!do_move_opaque)&&(!Scr.gs.EmulateMWM))||(AddWindow))
    MoveOutline(xl, yt, Width - 1, Height - 1);

  DisplayPosition(tmp_win,xl,yt,True);

  while (!finished && bad_window != tmp_win->w)
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

    done = False;
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
	if(!do_move_opaque)
	  MoveOutline(0, 0, 0, 0);
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
	aborted = True;
	finished = True;
      }
      done = True;
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
	done = True;
	break;
      }
      if(((Event.xbutton.button == 2)&&(!Scr.gs.EmulateMWM))||
	 ((Event.xbutton.button == 1)&&(Scr.gs.EmulateMWM)&&
	  (Event.xbutton.state & ShiftMask)))
      {
	do_resize_too = True;
	/* Fallthrough to button-release */
      }
      else
      {
	/* Abort the move if
	 *  - the move started with a pressed button and another button
	 *    was pressed during the operation
	 *  - no button was pressed at the beginning and any button
	 *    except button 1 was pressed. */
	if (button_mask || (Event.xbutton.button != 1))
	{
	  if(!do_move_opaque)
	    MoveOutline(0, 0, 0, 0);
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
	  aborted = True;
	  finished = True;
	}
	done = True;
	break;
      }
    case ButtonRelease:
      if(!do_move_opaque)
	MoveOutline(0, 0, 0, 0);
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
      if (xl != xl_orig || yt != yt_orig || vx != Scr.Vx || vy != Scr.Vy)
      {
	/* only snap if the window actually moved! */
        DoSnapAttract(tmp_win, Width, Height, &xl, &yt);
      }

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

      done = True;
      finished = True;
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
	if(!do_move_opaque)
	  MoveOutline(xl, yt, Width - 1, Height - 1);
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

      done = True;
      break;

    default:
      break;
    } /* switch */
    if(!done)
    {
      if (!do_move_opaque)
	/* must undraw the rubber band in case the event causes some drawing */
	MoveOutline(0,0,0,0);
      DispatchEvent(False);
      if(!do_move_opaque)
	MoveOutline(xl, yt, Width - 1, Height - 1);
    }
    if (do_move_opaque && !IS_ICONIFIED(tmp_win) && !IS_SHADED(tmp_win))
    {
      /* send configure notify event for windows that care about their
       * location */
      XEvent client_event;
      int nx = xl + tmp_win->boundary_width;
      int ny = yt + tmp_win->boundary_width +
	((HAS_BOTTOM_TITLE(tmp_win)) ? 0 : tmp_win->title_g.height);


      /* Don't send anything if position didn't change */
      if (!sent_cn || cnx != nx || cny != ny)
      {
        cnx = nx;
        cny = ny;
        sent_cn = True;
        client_event.type = ConfigureNotify;
        client_event.xconfigure.display = dpy;
        client_event.xconfigure.event = tmp_win->w;
        client_event.xconfigure.window = tmp_win->w;
        client_event.xconfigure.x = nx;
	client_event.xconfigure.y = ny;
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
    }
    if (do_move_opaque)
    {
      if (!IS_ICONIFIED(tmp_win))
      {
	tmp_win_copy.frame_g.x = xl;
	tmp_win_copy.frame_g.y = yt;
      }
      /* only do this with opaque moves, (i.e. the server is not grabbed) */
      BroadcastConfig(M_CONFIGURE_WINDOW, &tmp_win_copy);
      FlushOutputQueues();
    }
  } /* while (!finished) */

  if (!Scr.gs.do_hide_position_window)
    XUnmapWindow(dpy,Scr.SizeWindow);
  if (aborted || bad_window == tmp_win->w)
  {
    if (vx != Scr.Vx || vy != Scr.Vy)
    {
      MoveViewport(vx, vy, False);
    }
    if (aborted && do_move_opaque)
    {
      XMoveWindow(dpy, move_w, x_bak, y_bak);
    }
    if (bad_window == tmp_win->w)
    {
      XUnmapWindow(dpy, move_w);
      XBell(dpy, 0);
    }
  }
  if (!do_resize_too)
    /* Don't wait for buttons to come up when user is placing a new window
     * and wants to resize it. */
    WaitForButtonsUp(True);
  SET_WINDOW_BEING_MOVED_OPAQUE(tmp_win, 0);
  bad_window = None;

  return do_resize_too;
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

  if (Scr.gs.do_hide_position_window)
    return;
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


static char *hide_options[] =
{
  "none",
  "move",
  "resize",
  NULL
};

void HideGeometryWindow(F_CMD_ARGS)
{
  char *token = PeekToken(action, NULL);

  Scr.gs.do_hide_position_window = 0;
  Scr.gs.do_hide_resize_window = 0;
  switch(GetTokenIndex(token, hide_options, 0, NULL))
  {
  case 0:
    break;
  case 1:
    Scr.gs.do_hide_position_window = 1;
    break;
  case 2:
    Scr.gs.do_hide_resize_window = 1;
    break;
  default:
    Scr.gs.do_hide_position_window = 1;
    Scr.gs.do_hide_resize_window = 1;
    break;
  }
  return;
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

static Pixmap XorPixmap = None;

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

  /* free up XorPixmap if neccesary */
  if (XorPixmap != None) {
    XFreePixmap(dpy, XorPixmap);
    XorPixmap = None;
  }
}


void SetXORPixmap(F_CMD_ARGS)
{
  char *PixmapName;
  Picture *xp;
  XGCValues gcv;
  unsigned long gcm;

  action = GetNextToken(action, &PixmapName);
  if(PixmapName == NULL)
  {
    /* return to default value. */
    SetXOR(eventp, w, tmp_win, context, "0", Module);
    return;
  }
  /* get the picture in the root visual, colorlimit is ignored because the
   * pixels will be freed */
  UseDefaultVisual();
  xp = GetPicture(dpy, Scr.Root, NULL, PixmapName, 0);
  if (xp == NULL) {
    fvwm_msg(ERR,"SetXORPixmap","Can't find pixmap %s", PixmapName);
    free(PixmapName);
    UseFvwmVisual();
    return;
  }
  free(PixmapName);
  /* free up old pixmap */
  if (XorPixmap != None)
    XFreePixmap(dpy, XorPixmap);

  /* make a copy of the picture pixmap */
  XorPixmap = XCreatePixmap(dpy, Scr.Root, xp->width, xp->height, Pdepth);
  XCopyArea(dpy, xp->picture, XorPixmap, DefaultGC(dpy, Scr.screen), 0, 0,
	    xp->width, xp->height, 0, 0);
  /* destroy picture and free colors */
  DestroyPicture(dpy, xp);
  UseFvwmVisual();

  /* create Graphics context */
  gcm = GCFunction|GCLineWidth|GCTile|GCFillStyle|GCSubwindowMode;
  gcv.subwindow_mode = IncludeInferiors;
  gcv.function = GXxor;
  /* line width of 1 is necessary for Exceed servers */
  gcv.line_width = 1;
  gcv.tile = XorPixmap;
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
  extern Window bad_window;
  Bool finished = False, done = False, abort = False;
  Bool do_resize_opaque;
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
  rectangle start_g;
  int ymotion = 0;
  int xmotion = 0;
  int was_maximized;
  unsigned edge_wrap_x;
  unsigned edge_wrap_y;
  int px;
  int py;
  int i;
  Bool called_from_title = False;

  bad_window = False;
  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_RESIZE, ButtonPress))
    return;

  if (tmp_win == NULL)
    return;

#if 0
fprintf(stderr,"resize window '%s'\n", tmp_win->name);
#endif
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
  if (was_maximized)
  {
    /* must redraw the buttons now so that the 'maximize' button does not stay
     * depressed. */
    DrawDecorations(
      tmp_win, DRAW_BUTTONS, (tmp_win == Scr.Hilite), True, None);
  }

  /* can't resize icons */
  if(IS_ICONIFIED(tmp_win))
    return;

  do_resize_opaque = (IS_SHADED(tmp_win)) ? False : DO_RESIZE_OPAQUE(tmp_win);
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
      SetupFrame(tmp_win, tmp_win->frame_g.x, tmp_win->frame_g.y,
		 drag->width, tmp_win->frame_g.height, False);
    }
    else
    {
      SetupFrame(tmp_win, tmp_win->frame_g.x, tmp_win->frame_g.y,
		 drag->width, drag->height, False);
    }
    DrawDecorations(tmp_win, DRAW_ALL, True, True, None);
    update_absolute_geometry(tmp_win);
    maximize_adjust_offset(tmp_win);
    ResizeWindow = None;
    return;
  }

  if (Scr.bo.InstallRootCmap)
    InstallRootColormap();
  else
    InstallFvwmColormap();

  if(!GrabEm(CRS_RESIZE, GRAB_NORMAL))
  {
    XBell(dpy, 0);
    return;
  }

  if (!do_resize_opaque)
    MyXGrabServer(dpy);

  /* handle problems with edge-wrapping while resizing */
  edge_wrap_x = Scr.flags.edge_wrap_x;
  edge_wrap_y = Scr.flags.edge_wrap_y;
  Scr.flags.edge_wrap_x = 0;
  Scr.flags.edge_wrap_y = 0;

  if (IS_SHADED(tmp_win))
  {
    if (HAS_BOTTOM_TITLE(tmp_win))
      drag->height = tmp_win->frame_g.height;
    else
    {
      if (was_maximized)
	drag->height = tmp_win->max_g.height;
      else
	drag->height = tmp_win->normal_g.height;
    }
    drag->x = tmp_win->frame_g.x;
    drag->y = tmp_win->frame_g.y;
    drag->width = tmp_win->frame_g.width;
  }
  else
    XGetGeometry(dpy, (Drawable) ResizeWindow, &JunkRoot,
		 &drag->x, &drag->y, (unsigned int *)&drag->width,
		 (unsigned int *)&drag->height, &JunkBW,&JunkDepth);

  orig->x = drag->x;
  orig->y = drag->y;
  orig->width = drag->width;
  orig->height = drag->height;
  start_g.x = drag->x;
  start_g.y = drag->y;
  start_g.width = drag->width;
  start_g.height = drag->height;
  ymotion=xmotion=0;

  /* pop up a resize dimensions window */
  if (!Scr.gs.do_hide_resize_window)
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

    /* Now find the place to warp to. We simply use the sectors drawn when we
     * start resizing the window. */
#if 0
    tx = orig->width / 10 - 1;
    ty = orig->height / 10 - 1;
#else
    tx = 0;
    ty = 0;
#endif
    tx = max(tmp_win->boundary_width, tx);
    ty = max(tmp_win->boundary_width, ty);
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
	  wy = py;
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
	  wy = py;
	}
      }
      else
      {
	if (py < ty)
	{
	  ymotion = 1;
	  wx = orig->width/2;
	  wy = 0;
	  wx = px;
	}
	else if (dy < ty)
	{
	  ymotion = -1;
	  wx = orig->width/2;
	  wy = orig->height -1;
	  wx = px;
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
  if (!do_resize_opaque)
    MoveOutline(drag->x, drag->y, drag->width - 1, drag->height - 1);
  /* kick off resizing without requiring any motion if invoked with a key
   * press */
  if (eventp->type == KeyPress)
  {
    XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		  &stashed_x,&stashed_y,&JunkX, &JunkY, &JunkMask);
    DoResize(stashed_x, stashed_y, tmp_win, drag, orig, &xmotion, &ymotion,
	     do_resize_opaque);
  }
  else
    stashed_x = stashed_y = -1;

  /* loop to resize */
  while(!finished && bad_window != tmp_win->w)
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

    done = False;
    /* Handle a limited number of key press events to allow mouseless
     * operation */
    if(Event.type == KeyPress)
      Keyboard_shortcuts(&Event, tmp_win, ButtonRelease);
    switch(Event.type)
    {
    case ButtonPress:
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
      done = True;
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
	fButtonAbort = True;
    case KeyPress:
      /* simple code to bag out of move - CKH */
      if (XLookupKeysym(&(Event.xkey),0) == XK_Escape || fButtonAbort)
      {
	abort = True;
	finished = True;
	/* return pointer if aborted resize was invoked with key */
	if (stashed_x >= 0)
	{
	  XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, stashed_x,
		       stashed_y);
	}
	if (do_resize_opaque)
	{
	  DoResize(
	    start_g.x, start_g.y, tmp_win, &start_g, orig, &xmotion, &ymotion,
	    do_resize_opaque);
	  DrawDecorations(tmp_win, DRAW_ALL, True, True, None);
	}
      }
      done = True;
      break;

    case ButtonRelease:
      finished = True;
      done = True;
      break;

    case MotionNotify:
      if (!fForceRedraw)
      {
	x = Event.xmotion.x_root;
	y = Event.xmotion.y_root;
	/* resize before paging request to prevent resize from lagging
	 * mouse - mab */
	DoResize(
	  x, y, tmp_win, drag, orig, &xmotion, &ymotion, do_resize_opaque);
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

	DoResize(
	  x, y, tmp_win, drag, orig, &xmotion, &ymotion, do_resize_opaque);
      }
      fForceRedraw = False;
      done = True;
    default:
      break;
    }
    if(!done)
    {
      if (!do_resize_opaque)
	/* must undraw the rubber band in case the event causes some drawing */
	MoveOutline(0,0,0,0);
      DispatchEvent(False);
      if (!do_resize_opaque)
	MoveOutline(drag->x, drag->y, drag->width - 1, drag->height - 1);
    }
    else
    {
      if (do_resize_opaque)
      {
	/* only do this with opaque resizes, (i.e. the server is not grabbed)
	 */
	BroadcastConfig(M_CONFIGURE_WINDOW, tmp_win);
	FlushOutputQueues();
      }
    }
  }

  /* erase the rubber-band */
  if (!do_resize_opaque)
    MoveOutline(0, 0, 0, 0);

  /* pop down the size window */
  if (!Scr.gs.do_hide_resize_window)
    XUnmapWindow(dpy, Scr.SizeWindow);

  if(!abort && bad_window != tmp_win->w)
  {
    /* size will be >= to requested */
    ConstrainSize(
      tmp_win, &drag->width, &drag->height, xmotion, ymotion, True);
    if (IS_SHADED(tmp_win))
    {
      if (HAS_BOTTOM_TITLE(tmp_win))
      {
	SetupFrame(tmp_win, drag->x, tmp_win->frame_g.y,
		   drag->width, tmp_win->frame_g.height, False);
      }
      else
      {
	SetupFrame(tmp_win, drag->x, drag->y,
		   drag->width, tmp_win->frame_g.height, False);
      }
      tmp_win->normal_g.height = drag->height;
    }
    else
    {
      SetupFrame(
	tmp_win, drag->x, drag->y, drag->width, drag->height, False);
    }
  }
  if (abort && was_maximized)
  {
    /* since we aborted the resize, the window is still maximized */
    SET_MAXIMIZED(tmp_win, 1);
    /* force redraw */
    DrawDecorations(
      tmp_win, DRAW_BUTTONS, (tmp_win == Scr.Hilite), True, None);
  }

  if (bad_window == tmp_win->w)
  {
    XUnmapWindow(dpy, tmp_win->frame);
    XBell(dpy, 0);
  }

  if (Scr.bo.InstallRootCmap)
    UninstallRootColormap();
  else
    UninstallFvwmColormap();
  ResizeWindow = None;
  if (!do_resize_opaque)
    MyXUngrabServer(dpy);
  xmotion = 0;
  ymotion = 0;
  WaitForButtonsUp(True);
  UngrabEm(GRAB_NORMAL);
  Scr.flags.edge_wrap_x = edge_wrap_x;
  Scr.flags.edge_wrap_y = edge_wrap_y;
  update_absolute_geometry(tmp_win);
  maximize_adjust_offset(tmp_win);

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
static void DoResize(
  int x_root, int y_root, FvwmWindow *tmp_win, rectangle *drag,
  rectangle *orig, int *xmotionp, int *ymotionp, Bool do_resize_opaque)
{
  int action = 0;

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
    ConstrainSize(
      tmp_win, &drag->width, &drag->height, *xmotionp, *ymotionp, True);
    if (*xmotionp == 1)
      drag->x = orig->x + orig->width - drag->width;
    if (*ymotionp == 1)
      drag->y = orig->y + orig->height - drag->height;

    if (drag->x != tmp_win->frame_g.x ||
	drag->y != tmp_win->frame_g.y ||
	drag->width != tmp_win->frame_g.width ||
	drag->height != tmp_win->frame_g.height)
    {
      if(!do_resize_opaque)
      {
	MoveOutline(drag->x, drag->y, drag->width - 1, drag->height - 1);
      }
      else
      {
	SetupFrame(
	  tmp_win, drag->x, drag->y, drag->width, drag->height, False);
      }
    }
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

  if (Scr.gs.do_hide_resize_window)
    return;
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
void MoveOutline(int x, int  y, int  width, int height)
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

  XDrawRectangles(dpy, Scr.Root, Scr.DrawGC, rects, interleave * 5);

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

/* make sure a maximized window and it's normal version are never a page or
 * more apart. */
void maximize_adjust_offset(FvwmWindow *tmp_win)
{
  int off_x;
  int off_y;

  off_x = tmp_win->normal_g.x - tmp_win->max_g.x - tmp_win->max_offset.x;
  off_y = tmp_win->normal_g.y - tmp_win->max_g.y - tmp_win->max_offset.y;
  if (off_x >= Scr.MyDisplayWidth)
  {
    tmp_win->normal_g.x -=
      (off_x / Scr.MyDisplayWidth) * Scr.MyDisplayWidth;
  }
  else if (off_x <= Scr.MyDisplayWidth)
  {
    tmp_win->normal_g.x +=
      ((-off_x) / Scr.MyDisplayWidth) * Scr.MyDisplayWidth;
  }
  if (off_y >= Scr.MyDisplayHeight)
  {
    tmp_win->normal_g.y -=
      (off_y / Scr.MyDisplayHeight) * Scr.MyDisplayHeight;
  }
  else if (off_y <= Scr.MyDisplayHeight)
  {
    tmp_win->normal_g.y +=
      ((-off_y) / Scr.MyDisplayHeight) * Scr.MyDisplayHeight;
  }
}

/***********************************************************************
 *
 *  Procedure:
 *	(Un)Maximize a window.
 *
 ***********************************************************************/
void Maximize(F_CMD_ARGS)
{
  int page_x, page_y;
  int val1, val2, val1_unit, val2_unit;
  int toggle;
  char *token;
  char *taction;
  Bool grow_x = False;
  Bool grow_y = False;
  rectangle new_g;

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
  else
  {
    if (GetOnePercentArgument(token, &val1, &val1_unit) == 0)
    {
      val1 = 100;
      val1_unit = Scr.MyDisplayWidth;
    }
    else if (val1 < 0)
    {
      /* handle negative offsets */
      if (val1_unit == Scr.MyDisplayWidth)
      {
	val1 = 100 + val1;
      }
      else
      {
	val1 = Scr.MyDisplayWidth + val1;
      }
    }
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
  else
  {
    if (GetOnePercentArgument(token, &val2, &val2_unit) == 0)
    {
      val2 = 100;
      val2_unit = Scr.MyDisplayHeight;
    }
    else if (val2 < 0)
    {
      /* handle negative offsets */
      if (val2_unit == Scr.MyDisplayHeight)
      {
	val2 = 100 + val2;
      }
      else
      {
	val2 = Scr.MyDisplayHeight + val2;
      }
    }
  }

  if (IS_MAXIMIZED(tmp_win))
  {
#if 0
fprintf(stderr,"normalize window '%s'\n", tmp_win->name);
#endif
    SET_MAXIMIZED(tmp_win, 0);
    get_relative_geometry(&tmp_win->frame_g, &tmp_win->normal_g);
    if (IS_SHADED(tmp_win))
      get_shaded_geometry(tmp_win, &tmp_win->frame_g, &tmp_win->frame_g);
    ForceSetupFrame(
      tmp_win, tmp_win->frame_g.x, tmp_win->frame_g.y, tmp_win->frame_g.width,
      tmp_win->frame_g.height, True);
    DrawDecorations(tmp_win, DRAW_ALL, True, True, None);
  }
  else /* maximize */
  {
#if 0
fprintf(stderr,"maximize window '%s'\n", tmp_win->name);
#endif
    /* find the new page and geometry */
    new_g.x = tmp_win->frame_g.x;
    new_g.y = tmp_win->frame_g.y;
    new_g.width = tmp_win->frame_g.width;
    new_g.height = tmp_win->frame_g.height;
    if (IsRectangleOnThisPage(&tmp_win->frame_g, tmp_win->Desk))
    {
      /* maximize on visible page */
      page_x = 0;
      page_y = 0;
    }
    else
    {
      /* maximize on the page where the center of the window is */
      page_x = truncate_to_multiple(
	tmp_win->frame_g.x + tmp_win->frame_g.width / 2,
	Scr.MyDisplayWidth);
      page_y = truncate_to_multiple(
	tmp_win->frame_g.y + tmp_win->frame_g.height / 2,
	Scr.MyDisplayHeight);
    }

    /* handle command line arguments */
    if (grow_y)
    {
      MaximizeHeight(tmp_win, new_g.width, new_g.x, &new_g.height, &new_g.y);
    }
    else if(val2 > 0)
    {
      new_g.height = val2 * val2_unit / 100;
      new_g.y = page_y;
    }
    if (grow_x)
    {
      MaximizeWidth(tmp_win, &new_g.width, &new_g.x, new_g.height, new_g.y);
    }
    else if(val1 >0)
    {
      new_g.width = val1 * val1_unit / 100;
      new_g.x = page_x;
    }
    if(val1 ==0 && val2==0)
    {
      new_g.x = page_x;
      new_g.y = page_y;
      new_g.height = Scr.MyDisplayHeight;
      new_g.width = Scr.MyDisplayWidth;
    }
    /* now maximize it */
    SET_MAXIMIZED(tmp_win, 1);
    ConstrainSize(tmp_win, &new_g.width, &new_g.height, 0, 0, False);
    tmp_win->max_g = new_g;
    if (IS_SHADED(tmp_win))
      get_shaded_geometry(tmp_win, &new_g, &tmp_win->max_g);
    SetupFrame(
      tmp_win, new_g.x, new_g.y, new_g.width, new_g.height, True);
    DrawDecorations(tmp_win, DRAW_ALL, (Scr.Hilite == tmp_win), True, None);
    /* remember the offset between old and new position in case the maximized
     * window is moved more than the screen width/height. */
    update_absolute_geometry(tmp_win);
    tmp_win->max_offset.x = tmp_win->normal_g.x - tmp_win->max_g.x;
    tmp_win->max_offset.y = tmp_win->normal_g.y - tmp_win->max_g.y;
#if 0
fprintf(stderr,"%d %d %d %d, max_offset.x = %d, max_offset.y = %d\n", tmp_win->max_g.x, tmp_win->max_g.y, tmp_win->max_g.width, tmp_win->max_g.height, tmp_win->max_offset.x, tmp_win->max_offset.y);
#endif
  }
}

/* ----------------------------- stick code -------------------------------- */

void handle_stick(F_CMD_ARGS, int toggle)
{
  if ((toggle == 1 && IS_STICKY(tmp_win)) ||
      (toggle == 0 && !IS_STICKY(tmp_win)))
    return;

  if(IS_STICKY(tmp_win))
  {
#if 0
fprintf(stderr,"unstick window '%s'\n", tmp_win->name);
#endif
    SET_STICKY(tmp_win, 0);
    update_absolute_geometry(tmp_win);
    tmp_win->Desk = Scr.CurrentDesk;
#ifdef GNOME
    GNOME_SetDeskCount();
    GNOME_SetDesk(tmp_win);
#endif
  }
  else
  {
#if 0
fprintf(stderr,"stick window '%s'\n", tmp_win->name);
#endif
    if (tmp_win->Desk != Scr.CurrentDesk)
      do_move_window_to_desk(tmp_win, Scr.CurrentDesk);
    SET_STICKY(tmp_win, 1);
    if (!IsRectangleOnThisPage(&tmp_win->frame_g, Scr.CurrentDesk))
    {
      move_window_doit(eventp, w, tmp_win, context, "", Module, FALSE, TRUE);
      /* move_window_doit resets the STICKY flag, so we must set it after the
       * call! */
      SET_STICKY(tmp_win, 1);
    }
  }
  BroadcastConfig(M_CONFIGURE_WINDOW,tmp_win);
  DrawDecorations(tmp_win, DRAW_TITLE, (Scr.Hilite==tmp_win), True, None);

#ifdef GNOME
  GNOME_SetHints (tmp_win);
#endif
}

void stick_function(F_CMD_ARGS)
{
  int toggle;

  if (DeferExecution(eventp,&w,&tmp_win,&context,CRS_SELECT,ButtonRelease))
    return;

  toggle = ParseToggleArgument(action, &action, -1, 0);

  handle_stick(eventp, w, tmp_win, context, action, Module, toggle);
}
