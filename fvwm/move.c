/*
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/***********************************************************************
 *
 * code for moving windows
 *
 ***********************************************************************/

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <X11/keysym.h>
#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

extern XEvent Event;
extern int menuFromFrameOrWindowOrTitlebar;
Bool NeedToResizeToo;

/* Animated move stuff added by Greg J. Badros, gjb@cs.washington.edu */

float rgpctMovementDefault[32] = {
    -.01, 0, .01, .03,.08,.18,.3,.45,.60,.75,.85,.90,.94,.97,.99,1.0
    /* must end in 1.0 */
  };

int cmsDelayDefault = 10; /* milliseconds */

/* Perform the movement of the window. ppctMovement *must* have a 1.0 entry
 * somewhere in ins list of floats, and movement will stop when it hits a 1.0
 * entry */
void AnimatedMoveAnyWindow(FvwmWindow *tmp_win, Window w, int startX,
			   int startY, int endX, int endY,
			   Bool fWarpPointerToo, int cmsDelay,
			   float *ppctMovement )
{
  int pointerX, pointerY;
  int currentX, currentY;
  int lastX, lastY;
  int deltaX, deltaY;

  /* set our defaults */
  if (ppctMovement == NULL) ppctMovement = rgpctMovementDefault;
  if (cmsDelay < 0)         cmsDelay     = cmsDelayDefault;

  if (startX < 0 || startY < 0)
    {
    XGetGeometry(dpy, w, &JunkRoot, &currentX, &currentY,
		 &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
    if (startX < 0) startX = currentX;
    if (startY < 0) startY = currentY;
    }

  deltaX = endX - startX;
  deltaY = endY - startY;
  lastX = startX;
  lastY = startY;

  if (deltaX == 0 && deltaY == 0) return; /* go nowhere fast */

  do {
    currentX = startX + deltaX * (*ppctMovement);
    currentY = startY + deltaY * (*ppctMovement);
    XMoveWindow(dpy,w,currentX,currentY);
    if (fWarpPointerToo == TRUE) {
      XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &JunkX,&JunkY,&pointerX,&pointerY,&JunkMask);
      pointerX += currentX - lastX;
      pointerY += currentY - lastY;
      XWarpPointer(dpy,None,Scr.Root,0,0,0,0,
		   pointerX,pointerY);
    }
    if (tmp_win
#ifdef WINDOWSHADE
        && !(tmp_win->buttons & WSHADE)
#endif
)
    {
      /* send configure notify event for windows that care about their
       * location */
      XEvent client_event;
      client_event.type = ConfigureNotify;
      client_event.xconfigure.display = dpy;
      client_event.xconfigure.event = tmp_win->w;
      client_event.xconfigure.window = tmp_win->w;
      client_event.xconfigure.x = currentX + tmp_win->boundary_width;
      client_event.xconfigure.y = currentY + tmp_win->title_height +
	tmp_win->boundary_width;
      client_event.xconfigure.width = tmp_win->frame_width -
	2 * tmp_win->boundary_width;
      client_event.xconfigure.height = tmp_win->frame_height -
	2 * tmp_win->boundary_width - tmp_win->title_height;
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
      tmp_win->frame_x = currentX;
      tmp_win->frame_y = currentY;
      BroadcastConfig(M_CONFIGURE_WINDOW, tmp_win);
      FlushOutputQueues();
    }

    usleep(cmsDelay*1000); /* usleep takes microseconds */
#ifdef GJB_ALLOW_ABORTING_ANIMATED_MOVES
    /* this didn't work for me -- maybe no longer necessary since
       we warn the user when they use > .5 seconds as a between-frame delay
       time */
    if (XCheckMaskEvent(dpy,
			ButtonPressMask|ButtonReleaseMask|KeyPressMask,
			&Event)) {
      /* finish the move immediately */
      XMoveWindow(dpy,w,endX,endY);
      XFlush(dpy);
      return;
    }
#endif
    lastX = currentX;
    lastY = currentY;
    }
  while (*ppctMovement != 1.0 && ppctMovement++);

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

  /* gotta have a window */
  w = tmp_win->frame;
  if(tmp_win->flags & ICONIFIED)
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
	  tmp_win->flags &= ~STICKY;
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
		tmp_win->frame_width, tmp_win->frame_height,FALSE,False);
    if (fWarp & !fAnimated)
      XWarpPointer(dpy, None, None, 0, 0, 0, 0, FinalX - x, FinalY - y);
  }
  else /* icon window */
    {
      tmp_win->flags |= ICON_MOVED;
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

typedef struct _FvwmMoveable
{
  int w;
  int h;
  int x;
  int y;
} FvwmMoveable;

/* This function does the SnapAttraction stuff. If takes x and y coordinates
 * (*px and *py) and returns the snapped values. */
static void DoSnapAttract(FvwmWindow *tmp_win, int Width, int Height,
			  int *px, int *py)
{
  int nyt,nxl,dist,closestLeft,closestRight,closestBottom,closestTop;
  FvwmMoveable self, other;
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
  if(tmp_win->flags&ICONIFIED)
    {
      self.w = Width;
      self.h = Height;
    }
  else
    {
      self.w = Width;
      self.h = Height;
    }
  while(Scr.SnapAttraction >= 0 && tmp)
    {
      if(Scr.SnapMode == 0)  /* All */
	{
	  /* NOOP */
	}
      if(Scr.SnapMode == 1)  /* SameType */
	{
	  if( (tmp->flags&ICONIFIED) != (tmp_win->flags&ICONIFIED) )
	    { tmp = tmp->next; continue; }
	}
      if(Scr.SnapMode == 2)  /* Icons */
	{
	  if( !(tmp->flags&ICONIFIED) || !(tmp_win->flags&ICONIFIED) )
	    { tmp = tmp->next; continue; }
	}
      if(Scr.SnapMode == 3)  /* Windows */
	{
	  if( (tmp->flags&ICONIFIED) || (tmp_win->flags&ICONIFIED) )
	    { tmp = tmp->next; continue; }
	}
      if (tmp_win != tmp && (tmp_win->Desk == tmp->Desk))
	{
	  if(tmp->flags&ICONIFIED)
	    {
	      if(tmp->icon_p_height > 0)
		{
		  other.w = tmp->icon_p_width;
		  other.h = tmp->icon_p_height;
		}
	      else
		{
		  other.w = tmp->icon_w_width;
		  other.h = tmp->icon_w_height;
		}
	      other.x = tmp->icon_x_loc;
	      other.y = tmp->icon_y_loc;
	    }
	  else
	    {
	      other.w = tmp->frame_width;
	      other.h = tmp->frame_height;
	      other.x = tmp->frame_x;
	      other.y = tmp->frame_y;
	    }
	  if(!((other.y + other.h) < (*py) ||
	       (other.y) > (*py + self.h) ))
	    {
	      dist = abs(other.x - (*px + self.w));
              if(dist < closestRight)
		{
		  closestRight = dist;
		  if(((*px + self.w) >= other.x)&&
		     ((*px + self.w) < other.x+Scr.SnapAttraction))
		    nxl = other.x - self.w;
		  if(((*px + self.w) >= other.x - Scr.SnapAttraction)&&
		     ((*px + self.w) < other.x))
		    nxl = other.x - self.w;
                }
	      dist = abs(other.x + other.w - *px);
	      if(dist < closestLeft)
		{
		  closestLeft = dist;
		  if((*px <= other.x + other.w)&&
		     (*px > other.x + other.w - Scr.SnapAttraction))
		    nxl = other.x + other.w;
		  if((*px <= other.x + other.w + Scr.SnapAttraction)&&
		     (*px > other.x + other.w))
		    nxl = other.x + other.w;
		}
	    }
          /* ScreenEdges - SJL */
          if(!(Scr.MyDisplayHeight < (*py) ||
               (*py + self.h) < 0) && (Scr.SnapMode & 8))
            {
              dist = abs(Scr.MyDisplayWidth - (*px + self.w));

              if(dist < closestRight)
		{
		  closestRight = dist;

		  if(((*px + self.w) >= Scr.MyDisplayWidth)&&
		     ((*px + self.w) < Scr.MyDisplayWidth +
                      Scr.SnapAttraction))
		    nxl = Scr.MyDisplayWidth - self.w;

		  if(((*px + self.w) >= Scr.MyDisplayWidth -
                      Scr.SnapAttraction)&&
		     ((*px + self.w) < Scr.MyDisplayWidth))
		    nxl = Scr.MyDisplayWidth - self.w;
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

	  if(!((other.x + other.w) < (*px) || (other.x) > (*px + self.w)))
	    {
	      dist = abs(other.y - (*py + self.h));
	      if(dist < closestBottom)
		{
		  closestBottom = dist;
		  if(((*py + self.h) >= other.y)&&
		     ((*py + self.h) < other.y+Scr.SnapAttraction))
		    nyt = other.y - self.h;
		  if(((*py + self.h) >= other.y - Scr.SnapAttraction)&&
		     ((*py + self.h) < other.y))
		    nyt = other.y - self.h;
		}
	      dist = abs(other.y + other.h - *py);
	      if(dist < closestTop)
		{
		  closestTop = dist;
		  if((*py <= other.y + other.h)&&
		     (*py > other.y + other.h - Scr.SnapAttraction))
		    nyt = other.y + other.h;
		  if((*py <= other.y + other.h + Scr.SnapAttraction)&&
		     (*py > other.y + other.h))
		    nyt = other.y + other.h;
		}
	    }
          /* ScreenEdges - SJL */
          if (!(Scr.MyDisplayWidth < (*px) || (*px + self.w) < 0 )
              && (Scr.SnapMode & 8))
            {
              dist = abs(Scr.MyDisplayHeight - (*py + self.h));

	      if(dist < closestBottom)
		{
		  closestBottom = dist;
                  if(((*py + self.h) >= Scr.MyDisplayHeight)&&
                     ((*py + self.h) < Scr.MyDisplayHeight+Scr.SnapAttraction))
                    nyt = Scr.MyDisplayHeight - self.h;
                  if(((*py + self.h) >= Scr.MyDisplayHeight -
                      Scr.SnapAttraction)&&
                     ((*py + self.h) < Scr.MyDisplayHeight))
                    nyt = Scr.MyDisplayHeight - self.h;
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
  int xl,yt,delta_x,delta_y,paged;
  unsigned int button_mask = 0;
  FvwmWindow tmp_win_copy;
  int dx = Scr.EdgeScrollX ? Scr.EdgeScrollX : Scr.MyDisplayWidth;
  int dy = Scr.EdgeScrollY ? Scr.EdgeScrollY : Scr.MyDisplayHeight;

  /* make a copy of the tmp_win structure for sending to the pager */
  memcpy(&tmp_win_copy, tmp_win, sizeof(FvwmWindow));
  /* prevent flicker when paging */
  tmp_win->tmpflags.window_being_moved = opaque_move;

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
	      *FinalX = tmp_win->frame_x;
	      *FinalY = tmp_win->frame_y;
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
		  *FinalX = tmp_win->frame_x;
		  *FinalY = tmp_win->frame_y;
		  finished = TRUE;
		}
	      done = TRUE;
	      break;
	    }
	case ButtonRelease:
	  if(!opaque_move)
	    MoveOutline(Scr.Root, 0, 0, 0, 0);
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
		  if (tmp_win->flags & ICONIFIED)
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
		    XMoveWindow(dpy,tmp_win->frame,xl,yt);
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
      if (opaque_move && !(tmp_win->flags & ICONIFIED)
#ifdef WINDOWSHADE
           && !(tmp_win->buttons & WSHADE)
#endif
          )
        {
	  /* send configure notify event for windows that care about their
	   * location */
          XEvent client_event;
          client_event.type = ConfigureNotify;
          client_event.xconfigure.display = dpy;
          client_event.xconfigure.event = tmp_win->w;
          client_event.xconfigure.window = tmp_win->w;
          client_event.xconfigure.x = xl + tmp_win->boundary_width;
          client_event.xconfigure.y = yt + tmp_win->title_height +
	    tmp_win->boundary_width;
          client_event.xconfigure.width = Width - 2 * tmp_win->boundary_width;
          client_event.xconfigure.height = Height -
	    2 * tmp_win->boundary_width - tmp_win->title_height;
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
        tmp_win_copy.frame_x = xl;
        tmp_win_copy.frame_y = yt;
        BroadcastConfig(M_CONFIGURE_WINDOW, &tmp_win_copy);
        FlushOutputQueues();
      }
    }
  if (!NeedToResizeToo)
    /* Don't wait for buttons to come up when user is placing a new window
     * and wants to resize it. */
    WaitForButtonsUp();
  tmp_win->tmpflags.window_being_moved_opaque = 0;
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

void DisplayPosition (FvwmWindow *tmp_win, int x, int y,int Init)
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

  if(Scr.d_depth >= 2)
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


/****************************************************************************
 *
 * For menus, move, and resize operations, we can effect keyboard
 * shortcuts by warping the pointer.
 *
 ****************************************************************************/
void Keyboard_shortcuts(XEvent *Event, FvwmWindow *w, int ReturnEvent)
{
  int x,y,x_root,y_root;
  int x_move_size = 0, y_move_size = 0;
  int x_move,y_move;

  KeySym keysym;

  if (w)
    {
      x_move_size = w->hints.width_inc;
      y_move_size = w->hints.height_inc;
    }
  if (y_move_size < 5) y_move_size = 5;
  if (x_move_size < 5) x_move_size = 5;
  if(Event->xkey.state & ControlMask)
    x_move_size = y_move_size = 1;
  if(Event->xkey.state & ShiftMask)
    x_move_size = y_move_size = 100;

  keysym = XLookupKeysym(&Event->xkey,0);

  x_move = 0;
  y_move = 0;
  switch(keysym)
    {
    case XK_Up:
    case XK_KP_8:
    case XK_k:
    case XK_p:
      y_move = -y_move_size;
      break;
    case XK_Down:
    case XK_KP_2:
    case XK_n:
    case XK_j:
      y_move = y_move_size;
      break;
    case XK_Left:
    case XK_KP_4:
    case XK_b:
    case XK_h:
      x_move = -x_move_size;
      break;
    case XK_Right:
    case XK_KP_6:
    case XK_f:
    case XK_l:
      x_move = x_move_size;
      break;
    case XK_KP_1:
      x_move = -x_move_size;
      y_move = y_move_size;
      break;
    case XK_KP_3:
      x_move = x_move_size;
      y_move = y_move_size;
      break;
    case XK_KP_7:
      x_move = -x_move_size;
      y_move = -y_move_size;
      break;
    case XK_KP_9:
      x_move = x_move_size;
      y_move = -y_move_size;
      break;
    case XK_Return:
    case XK_KP_Enter:
    case XK_space:
      /* beat up the event */
      Event->type = ReturnEvent;
      break;
    case XK_Escape:
      /* simple code to bag out of move - CKH */
      /* return keypress event instead */
      Event->type = KeyPress;
      Event->xkey.keycode = XKeysymToKeycode(Event->xkey.display,keysym);
      break;
    default:
      break;
    }
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &Event->xany.window,
		&x_root, &y_root, &x, &y, &JunkMask);

  if((x_move != 0)||(y_move != 0))
    {
      /* beat up the event */
      XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, x_root+x_move,
		   y_root+y_move);

      /* beat up the event */
      Event->type = MotionNotify;
      Event->xkey.x += x_move;
      Event->xkey.y += y_move;
      Event->xkey.x_root += x_move;
      Event->xkey.y_root += y_move;
    }
}


void InteractiveMove(Window *win, FvwmWindow *tmp_win, int *FinalX,
		     int *FinalY, XEvent *eventp)
{
  int origDragX,origDragY,DragX, DragY, DragWidth, DragHeight;
  int XOffset, YOffset;
  Window w;

  Bool opaque_move = False;

  w = *win;

  InstallRootColormap();
  if (menuFromFrameOrWindowOrTitlebar)
    {
      /* warp the pointer to the cursor position from before menu appeared*/
      XFlush(dpy);
    }

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

  if((!opaque_move)&&(tmp_win->flags & ICONIFIED))
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
