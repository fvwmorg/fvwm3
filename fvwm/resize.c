
/****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/


/***********************************************************************
 *
 * window resizing borrowed from the "wm" window manager
 *
 ***********************************************************************/
#include "config.h"

#include <stdio.h>
#include <X11/keysym.h>
#include "fvwm.h"
#include "misc.h"
#include "screen.h"
#include "parse.h"

typedef struct geom
{
  int x;
  int y;
  int width;
  int height;
} geom;

/* DO NOT USE (STATIC) GLOBALS IN THIS MODULE!
 * Since some functions are called from other modules unwanted side effects
 * (i.e. bugs.) would be created */

extern int menuFromFrameOrWindowOrTitlebar;
extern Window PressedW;

static void DoResize(int x_root, int y_root, FvwmWindow *tmp_win,
		     geom *drag, geom *orig, int *xmotionp, int *ymotionp);

/****************************************************************************
 *
 * Starts a window resize operation
 *
 ****************************************************************************/
void resize_window(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		   unsigned long context, char *action,int *Module)
{
  Bool finished = FALSE, done = FALSE, abort = FALSE;
  int x,y,delta_x,delta_y,stashed_x,stashed_y;
  Window ResizeWindow;
  Bool flags;
  Bool fButtonAbort = False;
  int val1, val2, val1_unit,val2_unit,n;
  unsigned int button_mask = 0;
  geom sdrag;
  geom sorig;
  geom *drag = &sdrag;
  geom *orig = &sorig;
  int ymotion=0, xmotion = 0;

  if (DeferExecution(eventp,&w,&tmp_win,&context, MOVE, ButtonPress))
    return;

  if (tmp_win == NULL)
    return;

  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		 &JunkX, &JunkY, &JunkX, &JunkY, &button_mask);
  button_mask &= Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask;

  if(check_allowed_function2(F_RESIZE,tmp_win) == 0
#ifdef WINDOWSHADE
     || (tmp_win->buttons & WSHADE)
#endif
     )
    {
      XBell(dpy, 0);
      return;
    }
  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);

  tmp_win->flags &= ~MAXIMIZED;

  /* Already checked this in functions.c, but its here too incase
   * there's a resize on initial placement. */
  if(check_allowed_function2(F_RESIZE,tmp_win) == 0)
    {
      XBell(dpy, 0);
      return;
    }
  /* can't resize icons */
  if(tmp_win->flags & ICONIFIED)
    return;

  ResizeWindow = tmp_win->frame;

  if(n == 2)
    {
      drag->width = val1*val1_unit/100;
      drag->height = val2*val2_unit/100;
      drag->width += (2*tmp_win->boundary_width);
      drag->height += (tmp_win->title_height + 2*tmp_win->boundary_width);

      /* size will be less or equal to requested */
      ConstrainSize (tmp_win, &drag->width, &drag->height, False, xmotion,
		     ymotion);
      SetupFrame (tmp_win, tmp_win->frame_x,
		  tmp_win->frame_y ,drag->width, drag->height,FALSE);

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
  flags = Scr.flags;
  Scr.flags &= ~(EdgeWrapX|EdgeWrapY);

  XGetGeometry(dpy, (Drawable) ResizeWindow, &JunkRoot,
	       &drag->x, &drag->y, (unsigned int *)&drag->width,
	       (unsigned int *)&drag->height, &JunkBW,&JunkDepth);

  drag->x += tmp_win->bw;
  drag->y += tmp_win->bw;
  orig->x = drag->x;
  orig->y = drag->y;
  orig->width = drag->width;
  orig->height = drag->height;
  ymotion=xmotion=0;

  /* pop up a resize dimensions window */
  XMapRaised(dpy, Scr.SizeWindow);
  DisplaySize(tmp_win, orig->width, orig->height,True,True);

  /* Get the current position to determine which border to resize */
  if((PressedW != Scr.Root)&&(PressedW != None))
    {
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
  /* draw the rubber-band window */
  MoveOutline (Scr.Root, drag->x - tmp_win->bw, drag->y - tmp_win->bw,
	       drag->width - 1 + 2 * tmp_win->bw,
	       drag->height - 1 + 2 * tmp_win->bw);
  /* kick off resizing without requiring any motion if invoked with a key press */
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
      XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask | KeyPressMask |
		 ButtonMotionMask | PointerMotionMask | ExposureMask,  &Event);
      StashEventTime(&Event);

      if (Event.type == MotionNotify)
	/* discard any extra motion events before a release */
	while(XCheckMaskEvent(dpy, ButtonMotionMask |	ButtonReleaseMask |
			      PointerMotionMask,&Event))
	  {
	    StashEventTime(&Event);
	    if (Event.type == ButtonRelease) break;
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
	  x = Event.xmotion.x_root;
	  y = Event.xmotion.y_root;
	  /* resize before paging request to prevent resize from lagging
	   * mouse - mab */
	  DoResize(x, y, tmp_win, drag, orig, &xmotion, &ymotion);
	  /* need to move the viewport */
	  HandlePaging(Scr.EdgeScrollX,Scr.EdgeScrollY,&x,&y,
		       &delta_x,&delta_y,False);
	/* redraw outline if we paged - mab */
	if ( (delta_x != 0) || (delta_y != 0) )
	  {
	    orig->x -= delta_x;
	    orig->y -= delta_y;
	    drag->x -= delta_x;
	    drag->y -= delta_y;

	    DoResize(x, y, tmp_win, drag, orig, &xmotion, &ymotion);
	  }
	  done = TRUE;
	default:
	  break;
	}
      if(!done)
	{
	  MoveOutline(Scr.Root,0,0,0,0);
	  DispatchEvent();
	  MoveOutline(Scr.Root, drag->x - tmp_win->bw, drag->y - tmp_win->bw,
		      drag->width - 1 + 2 * tmp_win->bw,
		      drag->height - 1 + 2 * tmp_win->bw);

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
      SetupFrame (tmp_win, drag->x - tmp_win->bw,
		  drag->y - tmp_win->bw, drag->width, drag->height,FALSE);
    }
  UninstallRootColormap();
  ResizeWindow = None;
  MyXUngrabServer(dpy);
  UngrabEm();
  xmotion = 0;
  ymotion = 0;
  WaitForButtonsUp();

  Scr.flags |= flags & (EdgeWrapX|EdgeWrapY);
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
		     geom *drag, geom *orig, int *xmotionp, int *ymotionp)
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

      MoveOutline(Scr.Root, drag->x - tmp_win->bw,drag->y - tmp_win->bw,
		  drag->width - 1 + 2 * tmp_win->bw,
		  drag->height - 1 + 2 * tmp_win->bw);
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
void DisplaySize(FvwmWindow *tmp_win, int width, int height, Bool Init,
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

  dheight = height - tmp_win->title_height - 2*tmp_win->boundary_width;
  dwidth = width - 2*tmp_win->boundary_width;

  dwidth -= tmp_win->hints.base_width;
  dheight -= tmp_win->hints.base_height;
  dwidth /= tmp_win->hints.width_inc;
  dheight /= tmp_win->hints.height_inc;

  (void) sprintf (str, " %4d x %-4d ", dwidth, dheight);
  offset = (Scr.SizeStringWidth + SIZE_HINDENT*2
    - XTextWidth(Scr.StdFont.font,str,strlen(str)))/2;
  if(Init)
    {
      XClearWindow(dpy,Scr.SizeWindow);
      if(Scr.d_depth >= 2)
	RelieveWindow(tmp_win,
		      Scr.SizeWindow,0,0,Scr.SizeStringWidth+ SIZE_HINDENT*2,
		      Scr.StdFont.height + SIZE_VINDENT*2,
		      Scr.StdReliefGC,
		      Scr.StdShadowGC,FULL_HILITE);
    }
  else
    {
      XClearArea(dpy, Scr.SizeWindow, SIZE_HINDENT, SIZE_VINDENT,
		 Scr.SizeStringWidth, Scr.StdFont.height,False);
    }

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

void ConstrainSize (FvwmWindow *tmp_win, int *widthp, int *heightp,
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
    dheight -= (tmp_win->title_height + 2*tmp_win->boundary_width);

    minWidth = tmp_win->hints.min_width;
    minHeight = tmp_win->hints.min_height;

    baseWidth = tmp_win->hints.base_width;
    baseHeight = tmp_win->hints.base_height;

    maxWidth = tmp_win->hints.max_width;
    maxHeight =  tmp_win->hints.max_height;

/*    maxWidth = Scr.VxMax + Scr.MyDisplayWidth;
    maxHeight = Scr.VyMax + Scr.MyDisplayHeight;*/

    xinc = tmp_win->hints.width_inc;
    yinc = tmp_win->hints.height_inc;

    /*
     * First, clamp to min and max values
     */
    if (dwidth < minWidth) dwidth = minWidth;
    if (dheight < minHeight) dheight = minHeight;

    if (dwidth > maxWidth) dwidth = maxWidth;
    if (dheight > maxHeight) dheight = maxHeight;


    /*
     * Second, round to base + N * inc (up or down depending on resize type)
     */
    dwidth = ((dwidth - baseWidth + constrainx) / xinc * xinc) + baseWidth;
    dheight = ((dheight - baseHeight + constrainy) / yinc * yinc) + baseHeight;


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
      }

    /*
     * Fourth, account for border width and title height
     */
    *widthp = dwidth + 2*tmp_win->boundary_width;
    *heightp = dheight + tmp_win->title_height + 2*tmp_win->boundary_width;
#ifdef WINDOWSHADE
    if (tmp_win->buttons & WSHADE)
      *heightp = tmp_win->title_height + tmp_win->boundary_width;
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
  char draw;
  XRectangle rects[5];

  if (x == lastx && y == lasty && width == lastWidth && height == lastHeight)
    return;

  /* undraw the old one, if any */
  /* draw the new one, if any */
  draw = 0;
  while (1)
  {
    if (lastWidth || lastHeight)
    {
      int i;

      for (i=0; i < 4; i++)
      {
	rects[i].x = lastx + i;
	rects[i].y = lasty + i;
	rects[i].width = lastWidth - (i << 1);
	rects[i].height = lastHeight - (i << 1);
      }
      rects[3].y = lasty+3 + (lastHeight-6)/3;
      rects[3].height = (lastHeight-6)/3;
      rects[4].x = lastx+3 + (lastWidth-6)/3;
      rects[4].y = lasty+3;
      rects[4].width = (lastWidth-6)/3;
      rects[4].height = (lastHeight-6);
      XDrawRectangles(dpy,Scr.Root,Scr.DrawGC,rects,5);
    }
    draw++;

    if (draw < 2)
    {
      lastx = x;
      lasty = y;
      lastWidth = width;
      lastHeight = height;
    }
    else
      break;
  }
}
