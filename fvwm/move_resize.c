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
#include "parse.h"
#include "screen.h"
#include "module_interface.h"
#include "move_resize.h"

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
  if (eventp->type == ButtonPress || eventp->type == ButtonRelease)
    {
      DragX = eventp->xbutton.x_root;
      DragY = eventp->xbutton.y_root;
    }
  else
    XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild, &DragX, &DragY,
                  &JunkX, &JunkY, &JunkMask);

  if(!GrabEm(MOVE))
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
  do {
    currentX = startX + deltaX * (*ppctMovement);
    currentY = startY + deltaY * (*ppctMovement);
    if (lastX == currentX && lastY == currentY)
      /* don't waste time in the same spot */
      continue;
    XMoveWindow(dpy,w,currentX,currentY);
    if (fWarpPointerToo == TRUE) {
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
    XFlush(dpy);
    if (tmp_win) {
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
			&Event)) {
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

  if (DeferExecution(eventp,&w,&tmp_win,&context, MOVE,ButtonPress))
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
    if (fAnimated) {
      AnimatedMoveFvwmWindow(tmp_win,w,-1,-1,FinalX,FinalY,fWarp,-1,NULL);
    }
    SetupFrame (tmp_win, FinalX, FinalY,
		tmp_win->frame_g.width, tmp_win->frame_g.height,FALSE,False);
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
                      tmp_win->icon_w_width,
                      tmp_win->icon_w_height + tmp_win->icon_p_height);
      if (fAnimated) {
        AnimatedMoveOfWindow(tmp_win->icon_w,-1,-1,tmp_win->icon_xl_loc,
			     FinalY+tmp_win->icon_p_height, fWarp,-1,NULL);
      } else {
        XMoveWindow(dpy,tmp_win->icon_w, tmp_win->icon_xl_loc,
		    FinalY+tmp_win->icon_p_height);
        if (fWarp)
	  XWarpPointer(dpy, None, None, 0, 0, 0, 0, FinalX - x, FinalY - y);
      }
      if(tmp_win->icon_pixmap_w != None)
	{
	  XMapWindow(dpy,tmp_win->icon_w);
	  if (fAnimated) {
   	    AnimatedMoveOfWindow(tmp_win->icon_pixmap_w, -1,-1,
				 tmp_win->icon_x_loc,FinalY,fWarp,-1,NULL);
	  } else {
	    XMoveWindow(dpy, tmp_win->icon_pixmap_w, tmp_win->icon_x_loc,
			FinalY);
            if (fWarp)
	      XWarpPointer(dpy, None, None, 0, 0, 0, 0, FinalX - x,
			   FinalY - y);
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
static void DoSnapAttract(FvwmWindow *tmp_win, int Width, int Height,
			  int *px, int *py)
{
  int nyt,nxl,dist,closestLeft,closestRight,closestBottom,closestTop;
  rectangle self, other;
  FvwmWindow *tmp;

  /* START OF SNAPATTRACTION BLOCK, mirrored in ButtonRelease */
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
  if(IS_ICONIFIED(tmp_win))
    {
      self.width = Width;
      self.height = Height;
    }
  else
    {
      self.width = Width;
      self.height = Height;
    }
  while(Scr.SnapAttraction >= 0 && tmp)
    {
      if(Scr.SnapMode == 0)  /* All */
	{
	  /* NOOP */
	}
      if(Scr.SnapMode == 1)  /* SameType */
	{
	  if( IS_ICONIFIED(tmp) != IS_ICONIFIED(tmp_win))
	    {
	      tmp = tmp->next;
	      continue;
	    }
	}
      if(Scr.SnapMode == 2)  /* Icons */
	{
	  if( !IS_ICONIFIED(tmp) || !IS_ICONIFIED(tmp_win))
	    {
	      tmp = tmp->next;
	      continue;
	    }
	}
      if(Scr.SnapMode == 3)  /* Windows */
	{
	  if( IS_ICONIFIED(tmp) || IS_ICONIFIED(tmp_win))
	    {
	      tmp = tmp->next;
	      continue;
	    }
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
	  if(!((other.y + other.height) < (*py) ||
	       (other.y) > (*py + self.height) ))
	    {
	      dist = abs(other.x - (*px + self.width));
              if(dist < closestRight)
		{
		  closestRight = dist;
		  if(((*px + self.width) >= other.x)&&
		     ((*px + self.width) < other.x+Scr.SnapAttraction))
		    nxl = other.x - self.width;
		  if(((*px + self.width) >= other.x - Scr.SnapAttraction)&&
		     ((*px + self.width) < other.x))
		    nxl = other.x - self.width;
                }
	      dist = abs(other.x + other.width - *px);
	      if(dist < closestLeft)
		{
		  closestLeft = dist;
		  if((*px <= other.x + other.width)&&
		     (*px > other.x + other.width - Scr.SnapAttraction))
		    nxl = other.x + other.width;
		  if((*px <= other.x + other.width + Scr.SnapAttraction)&&
		     (*px > other.x + other.width))
		    nxl = other.x + other.width;
		}
	    }
          /* ScreenEdges - SJL */
          if(!(Scr.MyDisplayHeight < (*py) ||
               (*py + self.height) < 0) && (Scr.SnapMode & 8))
            {
              dist = abs(Scr.MyDisplayWidth - (*px + self.width));

              if(dist < closestRight)
		{
		  closestRight = dist;

		  if(((*px + self.width) >= Scr.MyDisplayWidth)&&
		     ((*px + self.width) < Scr.MyDisplayWidth +
                      Scr.SnapAttraction))
		    nxl = Scr.MyDisplayWidth - self.width;

		  if(((*px + self.width) >= Scr.MyDisplayWidth -
                      Scr.SnapAttraction)&&
		     ((*px + self.width) < Scr.MyDisplayWidth))
		    nxl = Scr.MyDisplayWidth - self.width;
                }

              dist = abs(*px);

	      if(dist < closestLeft)
		{
		  closestLeft = dist;

                  if((*px <= 0)&&
		     (*px > - Scr.SnapAttraction))
		    nxl = 0;
		  if((*px <= Scr.SnapAttraction)&&
		     (*px > 0))
		    nxl = 0;
                }
            }

	  if(!((other.x + other.width) < (*px) ||
	       (other.x) > (*px + self.width)))
	    {
	      dist = abs(other.y - (*py + self.height));
	      if(dist < closestBottom)
		{
		  closestBottom = dist;
		  if(((*py + self.height) >= other.y)&&
		     ((*py + self.height) < other.y+Scr.SnapAttraction))
		    nyt = other.y - self.height;
		  if(((*py + self.height) >= other.y - Scr.SnapAttraction)&&
		     ((*py + self.height) < other.y))
		    nyt = other.y - self.height;
		}
	      dist = abs(other.y + other.height - *py);
	      if(dist < closestTop)
		{
		  closestTop = dist;
		  if((*py <= other.y + other.height)&&
		     (*py > other.y + other.height - Scr.SnapAttraction))
		    nyt = other.y + other.height;
		  if((*py <= other.y + other.height + Scr.SnapAttraction)&&
		     (*py > other.y + other.height))
		    nyt = other.y + other.height;
		}
	    }
          /* ScreenEdges - SJL */
          if (!(Scr.MyDisplayWidth < (*px) || (*px + self.width) < 0 )
              && (Scr.SnapMode & 8))
            {
              dist = abs(Scr.MyDisplayHeight - (*py + self.height));

	      if(dist < closestBottom)
		{
		  closestBottom = dist;
                  if(((*py + self.height) >= Scr.MyDisplayHeight)&&
                     ((*py + self.height) < Scr.MyDisplayHeight +
		      Scr.SnapAttraction))
                    nyt = Scr.MyDisplayHeight - self.height;
                  if(((*py + self.height) >= Scr.MyDisplayHeight -
                      Scr.SnapAttraction)&&
                     ((*py + self.height) < Scr.MyDisplayHeight))
                    nyt = Scr.MyDisplayHeight - self.height;
                }

              dist = abs(- *py);

              if(dist < closestTop)
		{
                  closestTop = dist;
		  if((*py <= 0)&&
		     (*py > - Scr.SnapAttraction))
		    nyt = 0;
		  if((*py <=  Scr.SnapAttraction)&&
		     (*py > 0))
		    nyt = 0;

                }
            }
	}
      tmp = tmp->next;
    }
  if(nxl == -1)
    {
      if(*px != *px / Scr.SnapGridX * Scr.SnapGridX)
	{
	  *px = (*px+Scr.SnapGridX/2) / Scr.SnapGridX * Scr.SnapGridX;
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
	  *py = (*py+Scr.SnapGridY/2) / Scr.SnapGridY * Scr.SnapGridY;
	}
    }
  else
    {
      *py = nyt;
    }
  /* END OF SNAPATTRACTION BLOCK, mirrored in ButtonRelease */
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
      /* block until there is an interesting event */
      while (!XCheckMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
			      KeyPressMask | PointerMotionMask |
			      ButtonMotionMask | ExposureMask, &Event))
	{
	  if (HandlePaging(dx, dy, &xl,&yt, &delta_x,&delta_y,False,False))
	    {
	      /* Fake an event to force window reposition */
	      xl += XOffset;
	      yt += YOffset;
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
	  while(XCheckMaskEvent(dpy, PointerMotionMask | ButtonMotionMask |
				ButtonPressMask |ButtonRelease, &Event))
	    {
	      StashEventTime(&Event);
	      if(Event.type == ButtonRelease || Event.type == ButtonPress)
		break;
	    }
	}

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
	      *FinalX = tmp_win->frame_g.x;
	      *FinalY = tmp_win->frame_g.y;
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
		  *FinalX = tmp_win->frame_g.x;
		  *FinalY = tmp_win->frame_g.y;
		  finished = TRUE;
		}
	      done = TRUE;
	      break;
	    }
	case ButtonRelease:
	  if(!opaque_move)
	    MoveOutline(Scr.Root, 0, 0, 0, 0);
	  xl2 = Event.xmotion.x_root + XOffset;
	  yt2 = Event.xmotion.y_root + YOffset;
	  /* ignore the position of the button release if it was on a
	   * different page. */
	  if (!((xl < 0 && xl2 >= 0) || (xl >= 0 && xl2 < 0) ||
		(yt < 0 && yt2 >= 0) || (yt >= 0 && yt2 < 0)))
	  {
	    xl = xl2;
	    yt = yt2;
	  }
	  else
	  {
	    xl += XOffset;
	    yt += YOffset;
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
			XMoveWindow (dpy, tmp_win->icon_pixmap_w,
				     tmp_win->icon_x_loc,yt);
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
  		  HandlePaging(dx, dy, &xl, &yt, &delta_x, &delta_y, False,
			       False);
		  xl += XOffset;
		  yt += YOffset;
		  if ( (delta_x==0) && (delta_y==0))
		    /* break from while paged */
		    break;
		}
	      paged++;
	    }  /* end while paged */

	  done = TRUE;
	  break;

	default:
	  break;
	} /* switch */
      if(!done)
	{
	  DispatchEvent();
	  if(!opaque_move)
	    MoveOutline(Scr.Root, xl, yt, Width - 1, Height - 1);
	}
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
      if(opaque_move) { /* no point in doing this if server grabbed */
        tmp_win_copy.frame_g.x = xl;
        tmp_win_copy.frame_g.y = yt;
        BroadcastConfig(M_CONFIGURE_WINDOW, &tmp_win_copy);
        FlushOutputQueues();
      }
    }
  if (!NeedToResizeToo)
    /* Don't wait for buttons to come up when user is placing a new window
     * and wants to resize it. */
    WaitForButtonsUp();
  SET_WINDOW_BEING_MOVED_OPAQUE(tmp_win, 0);
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
    { /* just clear indside the relief lines to reduce flicker */
      XClearArea(dpy,Scr.SizeWindow,2,2,
                 Scr.SizeStringWidth + SIZE_HINDENT*2 - 3,
                 Scr.StdFont.height + SIZE_VINDENT*2 - 3,False);
    }

  if(Scr.depth >= 2)
    RelieveRectangle(dpy,Scr.SizeWindow,0,0,
                     Scr.SizeStringWidth+ SIZE_HINDENT*2 - 1,
                     Scr.StdFont.height + SIZE_VINDENT*2 - 1,
                     Scr.StdReliefGC,
                     Scr.StdShadowGC, 2);
  offset = (Scr.SizeStringWidth + SIZE_HINDENT*2
	    - XTextWidth(Scr.StdFont.font,str,strlen(str)))/2;
  XDrawString (dpy, Scr.SizeWindow, Scr.StdGC,
	       offset,
	       Scr.StdFont.font->ascent + SIZE_VINDENT,
	       str, strlen(str));
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
  int val1, val2, val1_unit,val2_unit,n;
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

  if (DeferExecution(eventp,&w,&tmp_win,&context, MOVE, ButtonPress))
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
  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);

  was_maximized = IS_MAXIMIZED(tmp_win);
  SET_MAXIMIZED(tmp_win, 0);

  /* can't resize icons */
  if(IS_ICONIFIED(tmp_win))
    return;

  if(n == 2)
    {
      drag->width = val1*val1_unit/100;
      drag->height = val2*val2_unit/100;
      drag->width += (2*tmp_win->boundary_width);
      drag->height += (tmp_win->title_g.height + 2*tmp_win->boundary_width);

      /* size will be less or equal to requested */
      ConstrainSize (tmp_win, &drag->width, &drag->height, False, xmotion,
		     ymotion);
      if (IS_SHADED(tmp_win))
	{
	  tmp_win->orig_g.width = drag->width;
	  tmp_win->orig_g.height = drag->height;
 	  SetupFrame (tmp_win, tmp_win->frame_g.x, tmp_win->frame_g.y,
		      drag->width, tmp_win->frame_g.height,FALSE,False);
	}
      else
	SetupFrame (tmp_win, tmp_win->frame_g.x, tmp_win->frame_g.y, drag->width,
		    drag->height,FALSE,False);
      SetBorder(tmp_win,True,True,True,None);

      ResizeWindow = None;
      return;
    }

  InstallRootColormap();

  if(!GrabEm(MOVE))
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
      if(PressedW == tmp_win->sides[1])  /* right */
	xmotion = -1;
      if(PressedW == tmp_win->sides[2])  /* bottom */
	ymotion = -1;
      if(PressedW == tmp_win->sides[3])  /* left */
	xmotion = 1;
      if(PressedW == tmp_win->corners[0])  /* upper-left */
	{
	  ymotion = 1;
	  xmotion = 1;
	}
      if(PressedW == tmp_win->corners[1])  /* upper-right */
	{
	  xmotion = -1;
	  ymotion = 1;
	}
      if(PressedW == tmp_win->corners[2]) /* lower left */
	{
	  ymotion = -1;
	  xmotion = 1;
	}
      if(PressedW == tmp_win->corners[3])  /* lower right */
	{
	  ymotion = -1;
	  xmotion = -1;
	}
    }
#ifndef NO_EXPERIMENTAL_RESIZE
  if((PressedW != Scr.Root)&&(xmotion == 0)&&(ymotion == 0))
    {
      int dx = orig->width - px;
      int dy = orig->height - py;
      int wx = -1;
      int wy = -1;
      int tx;
      int ty;

#ifdef DONT_USE_RESIZE_SECTORS
      tx = orig->width / 7 - 1;
      if (tx < 20)
      {
	tx = orig->width / 4 - 1;
	if (tx > 20)
	  tx = 20;
      }
      ty = orig->height / 7 - 1;
      if (ty < 20)
      {
	ty = orig->height / 4 - 1;
	if (ty > 20)
	  ty = 20;
      }
#else
      tx = orig->width / 3 - 1;
      ty = orig->height / 3 - 1;
#endif
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
	  XWarpPointer(dpy, None, ResizeWindow, 0, 0, 1, 1, wx, wy);
	  XFlush(dpy);
	}
    }
#endif

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
	  if (HandlePaging(Scr.EdgeScrollX,Scr.EdgeScrollY,&x,&y,
			   &delta_x,&delta_y,False,False))
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
	while(XCheckMaskEvent(dpy, ButtonMotionMask | ButtonReleaseMask |
			      PointerMotionMask, &Event))
	  {
	    StashEventTime(&Event);
	    if (Event.type == ButtonRelease)
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
	      HandlePaging(Scr.EdgeScrollX,Scr.EdgeScrollY,&x,&y,
			   &delta_x,&delta_y,False,False);
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
	  DispatchEvent();
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
      ConstrainSize (tmp_win, &drag->width, &drag->height, True, xmotion,
		     ymotion);
       if (IS_SHADED(tmp_win))
       {
         SetupFrame (tmp_win, tmp_win->frame_g.x, tmp_win->frame_g.y,
                     drag->width, tmp_win->frame_g.height, FALSE, False);
         tmp_win->orig_g.height = drag->height;
       }
      else
	SetupFrame (tmp_win, drag->x, drag->y, drag->width, drag->height,
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
      ConstrainSize (tmp_win, &drag->width, &drag->height, True, *xmotionp,
		     *ymotionp);
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
    { /* just clear indside the relief lines to reduce flicker */
      XClearArea(dpy,Scr.SizeWindow,2,2,
                 Scr.SizeStringWidth + SIZE_HINDENT*2 - 3,
                 Scr.StdFont.height + SIZE_VINDENT*2 - 3,False);
    }

  if(Scr.depth >= 2)
    RelieveRectangle(dpy,Scr.SizeWindow,0,0,
                     Scr.SizeStringWidth+ SIZE_HINDENT*2 - 1,
                     Scr.StdFont.height + SIZE_VINDENT*2 - 1,
                     Scr.StdReliefGC,
                     Scr.StdShadowGC, 2);
  offset = (Scr.SizeStringWidth + SIZE_HINDENT*2
    - XTextWidth(Scr.StdFont.font,str,strlen(str)))/2;
  XDrawString (dpy, Scr.SizeWindow, Scr.StdGC,
	       offset, Scr.StdFont.font->ascent + SIZE_VINDENT, str, 13);
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

void ConstrainSize(FvwmWindow *tmp_win, int *widthp, int *heightp,
		   Bool roundUp, int xmotion, int ymotion)
{
#define makemult(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
#define _min(a,b) (((a) < (b)) ? (a) : (b))
    int minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
    int baseWidth, baseHeight;
    int dwidth = *widthp, dheight = *heightp;
    int constrainx, constrainy;

    /* roundUp is True if called from an interactive resize */
    if (roundUp)
      {
	constrainx = tmp_win->hints.width_inc - 1;
	constrainy = tmp_win->hints.height_inc - 1;
      }
    else
      {
	constrainx = 0;
	constrainy = 0;
      }
    dwidth -= 2 *tmp_win->boundary_width;
    dheight -= (tmp_win->title_g.height + 2*tmp_win->boundary_width);

    minWidth = tmp_win->hints.min_width;
    minHeight = tmp_win->hints.min_height;

    maxWidth = tmp_win->hints.max_width;
    maxHeight =  tmp_win->hints.max_height;

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
     */
    dwidth = ((dwidth - baseWidth + constrainx) / xinc * xinc) + baseWidth;
    dheight = ((dheight - baseHeight + constrainy) / yinc * yinc) + baseHeight;

    /*
     * Step 2a: Check that we didn't violate min and max.
     */
    if (dwidth < minWidth) dwidth += xinc;
    if (dheight < minHeight) dheight += yinc;
    if (dwidth > maxWidth) dwidth -= xinc;
    if (dheight > maxHeight) dheight -= yinc;

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
	    delta = makemult(minAspectX * dheight / minAspectY - dwidth,
			     xinc);
	    if (dwidth + delta <= maxWidth)
	      dwidth += delta;
	  }
	if (minAspectX * dheight > minAspectY * dwidth)
	  {
	    delta = makemult(dheight - dwidth*minAspectY/minAspectX,
			     yinc);
	    if (dheight - delta >= minHeight)
	      dheight -= delta;
	    else
	      {
		delta = makemult(minAspectX*dheight / minAspectY - dwidth,
				 xinc);
		if (dwidth + delta <= maxWidth)
		  dwidth += delta;
	      }
	  }

        if ((maxAspectX * dheight < maxAspectY * dwidth)&&(ymotion == 0))
	  {
            delta = makemult(dwidth * maxAspectY / maxAspectX - dheight,
                             yinc);
            if (dheight + delta <= maxHeight)
	      dheight += delta;
	  }
        if ((maxAspectX * dheight < maxAspectY * dwidth))
	  {
	    delta = makemult(dwidth - maxAspectX*dheight/maxAspectY,
			     xinc);
	    if (dwidth - delta >= minWidth)
		dwidth -= delta;
	    else
	      {
		delta = makemult(dwidth * maxAspectY / maxAspectX - dheight,
				 yinc);
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
#if 0
    if (is_SHADED(tmp_win))
      *heightp = tmp_win->title_g.height + tmp_win->boundary_width;
#endif

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
}
