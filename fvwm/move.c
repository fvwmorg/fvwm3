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
void AnimatedMoveOfWindow(Window w,int startX,int startY,int endX, int endY,
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
    XFlush(dpy);
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


/****************************************************************************
 *
 * Start a window move operation
 *
 ****************************************************************************/
void move_window_doit(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context, char *action,int* Module,
		      Bool fAnimated, Bool fMoveToPage)
{
  int FinalX, FinalY;
  int n;
  int x,y;
  int width, height;
  int page[2];
  Bool fWarp = FALSE;

  if (DeferExecution(eventp,&w,&tmp_win,&context, MOVE,ButtonPress))
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
      if (GetIntegerArguments(action, NULL, page, 2) == 2)
	{
	  if (page[0] < 0 || page[1] < 0 ||
	      page[0]*Scr.MyDisplayWidth > Scr.VxMax ||
	      page[1]*Scr.MyDisplayHeight > Scr.VyMax)
	    {
	      fvwm_msg(ERR, "move_window_doit",
		       "MoveToPage: invalid page number");
	      return;
	    }
	  tmp_win->flags &= ~STICKY;
	  FinalX += page[0]*Scr.MyDisplayWidth - Scr.Vx;
	  FinalY += page[1]*Scr.MyDisplayHeight - Scr.Vy;
	}
    }
  else
    {
      n = GetMoveArguments(action,x,y,width+tmp_win->bw,height+tmp_win->bw,
                           &FinalX,&FinalY,&fWarp);
      if (n != 2)
	InteractiveMove(&w,tmp_win,&FinalX,&FinalY,eventp);
    }

  if (w == tmp_win->frame)
  {
    if (fAnimated) {
      AnimatedMoveOfWindow(w,-1,-1,FinalX,FinalY,fWarp,-1,NULL);
    }
    SetupFrame (tmp_win, FinalX, FinalY,
		tmp_win->frame_width, tmp_win->frame_height,FALSE);
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

void move_window(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		 unsigned long context, char *action,int* Module)
{
  move_window_doit(eventp,w,tmp_win,context,action,Module,FALSE,FALSE);
}

void animated_move_window(XEvent *eventp,Window w,FvwmWindow *tmp_win,
			  unsigned long context, char *action,int* Module)
{
  move_window_doit(eventp,w,tmp_win,context,action,Module,TRUE,FALSE);
}

void move_window_to_page(XEvent *eventp,Window w,FvwmWindow *tmp_win,
			 unsigned long context, char *action,int* Module)
{
  move_window_doit(eventp,w,tmp_win,context,action,Module,FALSE,TRUE);
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
  Window root, parent, *children;
  FvwmWindow *tmp;
  unsigned int nchildren;
  int i,j;
  int nyt,nxl,dist,closestLeft,closestRight,closestBottom,closestTop;
  Bool finished = False;
  Bool done;
  int xl,yt,delta_x,delta_y,paged;
  unsigned int button_mask = 0;

  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,&xl, &yt,
		&JunkX, &JunkY, &button_mask);
  button_mask &= Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask;
  xl += XOffset;
  yt += YOffset;

  if(((!opaque_move)&&(Scr.DefaultMenuFace->style!=MWMMenu))||(AddWindow))
    MoveOutline(Scr.Root, xl, yt, Width,Height);

  DisplayPosition(tmp_win,xl+Scr.Vx,yt+Scr.Vy,True);

  while (!finished)
    {
      /* block until there is an interesting event */
      XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask | KeyPressMask |
		 PointerMotionMask | ButtonMotionMask | ExposureMask, &Event);
      StashEventTime(&Event);

      /* discard any extra motion events before a logical release */
      if (Event.type == MotionNotify)
	{
	  while(XCheckMaskEvent(dpy, PointerMotionMask | ButtonMotionMask |
				ButtonPressMask |ButtonRelease, &Event))
	    {
	      StashEventTime(&Event);
	      if(Event.type == ButtonRelease) break;
	    }
	}

      done = FALSE;
      /* Handle a limited number of key press events to allow mouseless
       * operation */
      if(Event.type == KeyPress)
	Keyboard_shortcuts(&Event,ButtonRelease);
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
	      done = 1;
	      break;
	    }
	  if(((Event.xbutton.button == 2)&&(Scr.DefaultMenuFace != MWMMenu))||
	     ((Event.xbutton.button == 1)&&(Scr.DefaultMenuFace == MWMMenu)&&
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
	      done = 1;
	      break;
	    }
	case ButtonRelease:
	  if(!opaque_move)
	    MoveOutline(Scr.Root, 0, 0, 0, 0);
	  xl = Event.xmotion.x_root + XOffset;
	  yt = Event.xmotion.y_root + YOffset;

	  /* resist based on window edges */
	  tmp = Scr.FvwmRoot.next;
	  closestTop = Scr.SnapAttraction;
	  closestBottom = Scr.SnapAttraction;
	  closestRight = Scr.SnapAttraction;
	  closestLeft = Scr.SnapAttraction;
	  nxl = xl;
	  nyt = yt;
	  while(tmp)
	    {
	      if (tmp_win != tmp && !(tmp->flags&ICONIFIED) &&
		  (tmp_win->Desk == tmp->Desk))
		{
		  if(!((tmp->frame_y + tmp->frame_height) < (yt) ||
		       (tmp->frame_y) > (yt + Height) ))
		    {
		      dist = abs(tmp->frame_x - (xl + Width));
		      if(dist < closestRight)
			{
			  closestRight = dist;
			  if(((xl + Width) >= tmp->frame_x)&&
			     ((xl + Width) < tmp->frame_x+Scr.SnapAttraction))
			    nxl = tmp->frame_x - Width - tmp_win->bw;
			  if(((xl + Width) >= tmp->frame_x
			      - Scr.SnapAttraction)&&
			     ((xl + Width) < tmp->frame_x))
			    nxl = tmp->frame_x - Width - tmp_win->bw;
			}
		      dist = abs(tmp->frame_x + tmp->frame_width - xl);
		      if(dist < closestLeft)
			{
			  closestLeft = dist;
			  if((xl <= tmp->frame_x + tmp->frame_width)&&
			     (xl > tmp->frame_x + tmp->frame_width
			      - Scr.SnapAttraction))
			    nxl = tmp->frame_x + tmp->frame_width;
			  if((xl <= tmp->frame_x + tmp->frame_width
			      + Scr.SnapAttraction)&&
			     (xl > tmp->frame_x + tmp->frame_width))
			    nxl = tmp->frame_x + tmp->frame_width;
			}
		    }
		  if(!(   (tmp->frame_x + tmp->frame_width) < (xl) ||
			  (tmp->frame_x) > (xl + Width) ))
		    {
		      dist = abs(tmp->frame_y - (yt + Height));
		      if(dist < closestBottom)
			{
			  closestBottom = dist;
			  if(((yt + Height) >= tmp->frame_y)&&
			     ((yt + Height) < tmp->frame_y+Scr.SnapAttraction))
			    nyt = tmp->frame_y - Height - tmp_win->bw;
			  if(((yt + Height) >= tmp->frame_y
			      - Scr.SnapAttraction)&&
			     ((yt + Height) < tmp->frame_y))
			    nyt = tmp->frame_y - Height - tmp_win->bw;
			}
		      dist = abs(tmp->frame_y + tmp->frame_height - yt);
		      if(dist < closestTop)
			{
			  closestTop = dist;
			  if((yt <= tmp->frame_y + tmp->frame_height)&&
			     (yt > tmp->frame_y + tmp->frame_height
			      - Scr.SnapAttraction))
			    nyt = tmp->frame_y + tmp->frame_height;
			  if((yt <= tmp->frame_y + tmp->frame_height
			      + Scr.SnapAttraction)&&
			     (yt > tmp->frame_y + tmp->frame_height))
			    nyt = tmp->frame_y + tmp->frame_height;
			}
		    }
#if 0
		  if(((yt + Height) >= Scr.MyDisplayHeight)&&
		     ((yt + Height) < Scr.MyDisplayHeight+Scr.MoveResistance))
		    yt = Scr.MyDisplayHeight - Height - tmp_win->bw;
		  if((yt <= 0)&&(yt > -Scr.MoveResistance))
		    yt = 0;
#endif
                }
	      tmp = tmp->next;
	    }
	  xl = nxl;
	  yt = nyt;

	  /* Resist moving windows over the edge of the screen! */
	  if(((xl + Width) >= Scr.MyDisplayWidth)&&
	     ((xl + Width) < Scr.MyDisplayWidth+Scr.MoveResistance))
	    xl = Scr.MyDisplayWidth - Width - tmp_win->bw;
	  if((xl <= 0)&&(xl > -Scr.MoveResistance))
	    xl = 0;
	  if(((yt + Height) >= Scr.MyDisplayHeight)&&
	     ((yt + Height) < Scr.MyDisplayHeight+Scr.MoveResistance))
	    yt = Scr.MyDisplayHeight - Height - tmp_win->bw;
	  if((yt <= 0)&&(yt > -Scr.MoveResistance))
	    yt = 0;

	  *FinalX = xl;
	  *FinalY = yt;

	  done = TRUE;
	  finished = TRUE;
	  break;

	case MotionNotify:
	  xl = Event.xmotion.x_root;
	  yt = Event.xmotion.y_root;
/*	  HandlePaging(Scr.MyDisplayWidth,Scr.MyDisplayHeight,&xl,&yt,
		       &delta_x,&delta_y,False);  mab */
	  /* redraw the rubberband */
	  xl += XOffset;
	  yt += YOffset;

         /* resist based on window edges */
         tmp = Scr.FvwmRoot.next;
         closestTop = Scr.SnapAttraction;
         closestBottom = Scr.SnapAttraction;
         closestRight = Scr.SnapAttraction;
         closestLeft = Scr.SnapAttraction;
         nxl = xl;
         nyt = yt;
         while(tmp)
	 {
	   if (tmp_win != tmp && !(tmp->flags&ICONIFIED) &&
	       (tmp_win->Desk == tmp->Desk))
	   {
	     if(!(   (tmp->frame_y + tmp->frame_height) < (yt) ||
		     (tmp->frame_y) > (yt + Height) ))
	     {
	       dist = abs(tmp->frame_x - (xl + Width));
	       if(dist < closestRight)
	       {
		 closestRight = dist;
		 if(((xl + Width) >= tmp->frame_x)&&
		    ((xl + Width) < tmp->frame_x+Scr.SnapAttraction))
		   nxl = tmp->frame_x - Width - tmp_win->bw;
		 if(((xl + Width) >= tmp->frame_x-Scr.SnapAttraction)&&
		    ((xl + Width) < tmp->frame_x))
		   nxl = tmp->frame_x - Width - tmp_win->bw;
	       }
	       dist = abs(tmp->frame_x + tmp->frame_width - xl);
	       if(dist < closestLeft)
	       {
		 closestLeft = dist;
		 if((xl <= tmp->frame_x + tmp->frame_width)&&
		    (xl > tmp->frame_x + tmp->frame_width-Scr.SnapAttraction))
		   nxl = tmp->frame_x + tmp->frame_width;
		 if((xl <= tmp->frame_x + tmp->frame_width +
		     Scr.SnapAttraction)&&
		    (xl > tmp->frame_x + tmp->frame_width))
		   nxl = tmp->frame_x + tmp->frame_width;
	       }
	     }
	     if(!((tmp->frame_x + tmp->frame_width) < (xl) ||
		  (tmp->frame_x) > (xl + Width) ))
	     {
	       dist = abs(tmp->frame_y - (yt + Height));
	       if(dist < closestBottom)
	       {
		 closestBottom = dist;
		 if(((yt + Height) >= tmp->frame_y)&&
		    ((yt + Height) < tmp->frame_y+Scr.SnapAttraction))
		   nyt = tmp->frame_y - Height - tmp_win->bw;
		 if(((yt + Height) >= tmp->frame_y-Scr.SnapAttraction)&&
		    ((yt + Height) < tmp->frame_y))
		   nyt = tmp->frame_y - Height - tmp_win->bw;
	       }
	       dist = abs(tmp->frame_y + tmp->frame_height - yt);
	       if(dist < closestTop)
	       {
		 closestTop = dist;
		 if((yt <= tmp->frame_y + tmp->frame_height)&&
		    (yt > tmp->frame_y + tmp->frame_height-Scr.SnapAttraction))
		   nyt = tmp->frame_y + tmp->frame_height;
		 if((yt <= tmp->frame_y + tmp->frame_height +
		     Scr.SnapAttraction)&&
		    (yt > tmp->frame_y + tmp->frame_height))
		   nyt = tmp->frame_y + tmp->frame_height;
	       }
	     }
#if 0
	     if(((yt + Height) >= Scr.MyDisplayHeight)&&
		((yt + Height) < Scr.MyDisplayHeight+Scr.MoveResistance))
	       yt = Scr.MyDisplayHeight - Height - tmp_win->bw;
	     if((yt <= 0)&&(yt > -Scr.MoveResistance))
	       yt = 0;
#endif
	   }
	   tmp = tmp->next;
	 }
         xl = nxl;
         yt = nyt;

	 /* Resist moving windows over the edge of the screen! */
	 if(((xl + Width) >= Scr.MyDisplayWidth)&&
	    ((xl + Width) < Scr.MyDisplayWidth+Scr.MoveResistance))
	   xl = Scr.MyDisplayWidth - Width - tmp_win->bw;
	 if((xl <= 0)&&(xl > -Scr.MoveResistance))
	   xl = 0;
	 if(((yt + Height) >= Scr.MyDisplayHeight)&&
	    ((yt + Height) < Scr.MyDisplayHeight+Scr.MoveResistance))
	   yt = Scr.MyDisplayHeight - Height - tmp_win->bw;
	 if((yt <= 0)&&(yt > -Scr.MoveResistance))
	   yt = 0;

	 /* check Paging request once and only once after outline redrawn */
	 /* redraw after paging if needed - mab */
	 paged=0;
	 while(paged<=1)
	 {
	   if(!opaque_move)
	     MoveOutline(Scr.Root, xl, yt, Width,Height);
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
	   DisplayPosition(tmp_win,xl+Scr.Vx,yt+Scr.Vy,False);

	   /* prevent window from lagging behind mouse when paging - mab */
	   if(paged==0)
	   {
	     xl = Event.xmotion.x_root;
	     yt = Event.xmotion.y_root;
#if 0
	     HandlePaging(Scr.MyDisplayWidth,Scr.MyDisplayHeight,&xl,&yt,
			  &delta_x,&delta_y,False);
#else /* probably should actually use EdgeScroll values: */
	     HandlePaging(Scr.EdgeScrollX,Scr.EdgeScrollY,&xl,&yt,
			  &delta_x,&delta_y,False);
#endif
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
	}
      if(!done)
      {
	if(!opaque_move)
	  MoveOutline(Scr.Root,0,0,0,0);
	DispatchEvent();
	if(!opaque_move)
	  MoveOutline(Scr.Root, xl, yt, Width, Height);
      }
    }
  WaitForButtonsUp();
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
      if(Scr.d_depth >= 2)
	RelieveWindow(tmp_win,Scr.SizeWindow,0,0,
		      Scr.SizeStringWidth+ SIZE_HINDENT*2,
		      Scr.StdFont.height + SIZE_VINDENT*2,
		      Scr.DefaultMenuFace->MenuReliefGC,Scr.DefaultMenuFace->MenuShadowGC, FULL_HILITE);

    }
  else
    {
      XClearArea(dpy,Scr.SizeWindow,SIZE_HINDENT,SIZE_VINDENT,Scr.SizeStringWidth,
		 Scr.StdFont.height,False);
    }

  offset = (Scr.SizeStringWidth + SIZE_HINDENT*2
	    - XTextWidth(Scr.StdFont.font,str,strlen(str)))/2;
  XDrawString (dpy, Scr.SizeWindow, Scr.DefaultMenuFace->MenuGC,
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
void Keyboard_shortcuts(XEvent *Event, int ReturnEvent)
{
  int x,y,x_root,y_root;
  int x_move_size,y_move_size,x_move,y_move;
  KeySym keysym;

  if (!Scr.Hilite)
    return;
  /* Pick the size of the cursor movement, window resize increments are first
     choice followed by height of a menu item */
  x_move_size = Scr.Hilite->hints.width_inc;
  y_move_size = Scr.Hilite->hints.height_inc;
  if (y_move_size < 5) y_move_size = Scr.EntryHeight;
  if (x_move_size < 5) x_move_size = y_move_size;
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


void InteractiveMove(Window *win, FvwmWindow *tmp_win, int *FinalX, int *FinalY, XEvent *eventp)
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

  if (eventp->type != KeyPress)
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

  DragWidth += JunkBW;
  DragHeight+= JunkBW;
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
