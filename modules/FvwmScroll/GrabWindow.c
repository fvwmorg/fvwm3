/* This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

#define MAX_ICON_NAME_LEN 255

#include "config.h"

#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "libs/Module.h"
#include "libs/fvwmlib.h"
#include "libs/Picture.h"
#include "libs/ModGraph.h"

#include "FvwmScroll.h"

int Width = 300, Height = 300;
int target_width, target_height;
int target_x_offset = 0, target_y_offset = 0;
int exposed;
int Reduction_H = 2;
int Reduction_V = 2;

#define BAR_WIDTH 21
#define SCROLL_BAR_WIDTH 9

#define PAD_WIDTH2 3
#define PAD_WIDTH3 5

Window main_win,holder_win;
Pixel hilite_pix,shadow_pix;
extern char *BackColor;

#define MW_EVENTS   (ExposureMask | StructureNotifyMask| ButtonReleaseMask |\
		     ButtonPressMask | ButtonMotionMask | FocusChangeMask)

Atom wm_del_win;
Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_COLORMAP_WINDOWS;

/************************************************************************
 *
 * Sizes and creates the window 
 *
 ***********************************************************************/
XSizeHints mysizehints;
void CreateWindow(int x,int y, int w, int h)
{
  XGCValues gcv;
  unsigned long gcm;
  XSetWindowAttributes attributes;

  wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);
  _XA_WM_PROTOCOLS = XInternAtom (dpy, "WM_PROTOCOLS", False);

  mysizehints.flags = PWinGravity| PResizeInc | PBaseSize |
    PMaxSize | PMinSize | USSize | USPosition;
  /* subtract one for the right/bottom border */
  mysizehints.width_inc = 1;
  mysizehints.height_inc = 1;
  mysizehints.base_height = BAR_WIDTH+PAD_WIDTH3;
  mysizehints.base_width = BAR_WIDTH+PAD_WIDTH3;

  Width = w/Reduction_H + BAR_WIDTH + PAD_WIDTH3;
  Height = h/Reduction_V + BAR_WIDTH + PAD_WIDTH3;
  target_width = w;
  target_height = h;
  mysizehints.width = Width;
  mysizehints.height = Height;
  mysizehints.x = x;
  mysizehints.y = y;
  mysizehints.max_width = w + BAR_WIDTH + PAD_WIDTH3;
  mysizehints.max_height = h + BAR_WIDTH + PAD_WIDTH3;

  mysizehints.win_gravity = NorthWestGravity;	

  attributes.colormap = G->cmap;
  attributes.background_pixmap = None;
  attributes.border_pixel = 0;
  main_win = XCreateWindow(dpy, Root, mysizehints.x, mysizehints.y,
			   mysizehints.width, mysizehints.height, 0, G->bg->depth,
			   InputOutput, G->viz,
			   CWColormap | CWBackPixmap | CWBorderPixel,
			   &attributes);

  if (!G->useFvwmLook) {
    InitPictureCMap(dpy, main_win);
    G->bg->type.bits.is_pixmap = False;
    G->bg->pixmap = (Pixmap)GetColor(BackColor);
    hilite_pix = GetHilite((Pixel)G->bg->pixmap);
    shadow_pix = GetShadow((Pixel)G->bg->pixmap);
  }

  SetWindowBackground(dpy, main_win, mysizehints.width, mysizehints.height,
		      G->bg, G->shadowGC);
  XSetWMProtocols(dpy,main_win,&wm_del_win,1);

  XSetWMNormalHints(dpy,main_win,&mysizehints);
  XSelectInput(dpy,main_win,MW_EVENTS);
  change_window_name(MyName);

  holder_win = XCreateWindow(dpy, main_win, PAD_WIDTH3, PAD_WIDTH3,
			     mysizehints.width - BAR_WIDTH - PAD_WIDTH3,
			     mysizehints.height - BAR_WIDTH - PAD_WIDTH3,
			     0, G->bg->depth, InputOutput, G->viz,
			     CWColormap | CWBackPixmap | CWBorderPixel,
			     &attributes);
  XMapWindow(dpy,holder_win);

  if (!G->useFvwmLook) {
    gcm = GCForeground;
    gcv.foreground = hilite_pix;
    G->reliefGC = XCreateGC(dpy, holder_win, gcm, &gcv);
    gcv.foreground = shadow_pix;
    G->shadowGC = XCreateGC(dpy, holder_win, gcm, &gcv);
  }

  _XA_WM_COLORMAP_WINDOWS = XInternAtom (dpy, "WM_COLORMAP_WINDOWS", False);
 }

/****************************************************************************
 * 
 * Loads a single color
 *
 ****************************************************************************/ 
Pixel GetColor(char *name)
{
  XColor color;
  color.pixel = 0;
   if (!XParseColor (dpy, G->cmap, name, &color)) 
     {
       nocolor("parse",name);
     }
   else if(!XAllocColor (dpy, G->cmap, &color)) 
     {
       nocolor("alloc",name);
     }
  return color.pixel;
}

/***********************************************************************
 *
 *  Procedure:
 *	Loop - wait for data to process
 *
 ***********************************************************************/
#define RIGHT 7
#define LEFT 6
#define BOTTOM 5
#define TOP 4
#define QUIT 3
#define VERTICAL 2
#define HORIZONTAL 1
#define NONE 0
int motion = NONE;

void Loop(Window target)
{
  Window root;
  int x,y,depth,border_width;
  XEvent Event;
  int tw,th;
  char *temp;
  char *prop = NULL;
  Atom actual = None;
  int actual_format;
  unsigned long nitems, bytesafter;

  while(1) {
    fd_set fdset;

    FD_ZERO(&fdset);
    FD_SET(fd[1], &fdset);
    FD_SET(x_fd, &fdset);

    /* process all X events first */
    while (XPending(dpy)) {
      XNextEvent(dpy,&Event);
      switch(Event.type)
	{
	case Expose:
	  exposed = 1;
	  RedrawWindow(target);
	  break;
	  
	case ConfigureNotify:
	  XGetGeometry(dpy,main_win,&root,&x,&y,
		       (unsigned int *)&tw,(unsigned int *)&th,
		       (unsigned int *)&border_width,
		       (unsigned int *)&depth);
	  if((tw != Width)||(th!= Height))
	    {
	      XResizeWindow(dpy,holder_win,tw-BAR_WIDTH-PAD_WIDTH3,
			    th-BAR_WIDTH-PAD_WIDTH3);
	      Width = tw;
	      Height = th;
	      if(target_y_offset + Height - BAR_WIDTH > target_height)
		target_y_offset  = target_height - Height + BAR_WIDTH;
	      if(target_y_offset < 0)
		target_y_offset = 0;
	      if(target_x_offset < 0)
		target_x_offset = 0;
	      if(target_x_offset + Width - BAR_WIDTH > target_width)
		target_x_offset  = target_width - Width + BAR_WIDTH;

	      XMoveWindow(dpy,target,-target_x_offset, -target_y_offset);
	      exposed = 1;
	      RedrawWindow(target);
	    }
	  break;
	  
	case ButtonPress:
	  if((Event.xbutton.y > Height-BAR_WIDTH) &&
	     (Event.xbutton.x < SCROLL_BAR_WIDTH+PAD_WIDTH3))
	    {
	      motion = LEFT;
	      exposed = 2;
	      RedrawWindow(target);
	    }
	  else if((Event.xbutton.y > Height-BAR_WIDTH) &&
	     (Event.xbutton.x > Width-BAR_WIDTH-SCROLL_BAR_WIDTH-2) &&
	     (Event.xbutton.x < Width-BAR_WIDTH))
	    {
	      motion = RIGHT;
	      exposed = 2;
	      RedrawWindow(target);
	    }
	  else if((Event.xbutton.y < SCROLL_BAR_WIDTH+PAD_WIDTH3) &&
	     (Event.xbutton.x > Width-BAR_WIDTH))
	    {
	      motion = TOP;
	      exposed = 2;
	      RedrawWindow(target);
	    }
	  else if((Event.xbutton.y > Height-BAR_WIDTH-SCROLL_BAR_WIDTH-2) &&
	     (Event.xbutton.y < Height-BAR_WIDTH)&&
	     (Event.xbutton.x > Width-BAR_WIDTH))
	    {
	      motion = BOTTOM;
	      exposed = 2;
	      RedrawWindow(target);
	    }
	  else if((Event.xbutton.x > Width - BAR_WIDTH)&&
		  (Event.xbutton.y < Height- BAR_WIDTH))
	    {

	      motion = VERTICAL;
	      target_y_offset=(Event.xbutton.y-PAD_WIDTH3-SCROLL_BAR_WIDTH)*
		target_height/
		(Height-BAR_WIDTH-PAD_WIDTH3 - 2*SCROLL_BAR_WIDTH);
	      if(target_y_offset+Height-BAR_WIDTH -PAD_WIDTH3 > target_height)
		target_y_offset  = target_height - Height+BAR_WIDTH+PAD_WIDTH3;
	      if(target_y_offset < 0)
		target_y_offset = 0;
	      XMoveWindow(dpy,target,-target_x_offset, -target_y_offset);
	      RedrawWindow(target);
	    }
	  else if((Event.xbutton.y > Height- BAR_WIDTH ) &&
		  (Event.xbutton.x < Width- BAR_WIDTH))	    
	    {
	      motion=HORIZONTAL;
	      target_x_offset=(Event.xbutton.x -PAD_WIDTH3-SCROLL_BAR_WIDTH)*
		target_width/
		(Width-BAR_WIDTH-PAD_WIDTH3-2*SCROLL_BAR_WIDTH);
	      if(target_x_offset < 0)
		target_x_offset = 0;
	      
	      if(target_x_offset + Width - BAR_WIDTH -PAD_WIDTH3> target_width)
		target_x_offset  = target_width - Width + BAR_WIDTH+PAD_WIDTH3;
	      XMoveWindow(dpy,target,-target_x_offset, -target_y_offset);
	      RedrawWindow(target);
	    }
	  else if((Event.xbutton.y > Height- BAR_WIDTH ) &&
		  (Event.xbutton.x > Width- BAR_WIDTH))	    
	    {
	      exposed = 2;
	      motion=QUIT;
	    }
	  RedrawWindow(target);
	  break;

	case ButtonRelease:
	  if((Event.xbutton.y > Height- BAR_WIDTH ) &&
	    (Event.xbutton.x > Width- BAR_WIDTH)&&
	     (motion==QUIT))
	    {
	      XUnmapWindow(dpy,main_win);
	      XReparentWindow(dpy,target,Root,x,y);
	      XSync(dpy,0);
	      exit(0);
	    }
	  if((motion == LEFT)&&(Event.xbutton.y > Height-BAR_WIDTH) &&
	     (Event.xbutton.x < SCROLL_BAR_WIDTH+PAD_WIDTH3))
	    {
	      target_x_offset -= (Width-BAR_WIDTH-PAD_WIDTH2);
	      if(target_x_offset < 0)
		target_x_offset = 0;
	      XMoveWindow(dpy,target,-target_x_offset, -target_y_offset);
	      motion = NONE;
	      exposed = 2;
	    }
	  else if((motion == RIGHT)&&(Event.xbutton.y > Height-BAR_WIDTH) &&
	     (Event.xbutton.x > Width-BAR_WIDTH-SCROLL_BAR_WIDTH-2) &&
	     (Event.xbutton.x < Width-BAR_WIDTH))
	    {
	      target_x_offset += (Width-BAR_WIDTH-PAD_WIDTH2);
	      if(target_x_offset+Width-BAR_WIDTH -PAD_WIDTH3 > target_width)
		target_x_offset  = target_width - Width+BAR_WIDTH+PAD_WIDTH3;
	      XMoveWindow(dpy,target,-target_x_offset, -target_y_offset);
	      motion = NONE;
	      exposed = 2;
	    }
	  else if((motion == TOP)&&
		  (Event.xbutton.y<SCROLL_BAR_WIDTH+PAD_WIDTH3)&&
		  (Event.xbutton.x > Width-BAR_WIDTH))
	    {
	      target_y_offset -= (Height-BAR_WIDTH-PAD_WIDTH2);
	      if(target_y_offset < 0)
		target_y_offset = 0;
	      XMoveWindow(dpy,target,-target_x_offset, -target_y_offset);
	      motion = NONE;
	      exposed = 2;
	    }
	  else if((motion == BOTTOM)&&
		  (Event.xbutton.y > Height-BAR_WIDTH-SCROLL_BAR_WIDTH-2) &&
		  (Event.xbutton.y < Height-BAR_WIDTH)&&
		  (Event.xbutton.x > Width-BAR_WIDTH))
	    {
	      target_y_offset += (Height-BAR_WIDTH-PAD_WIDTH2);
	      if(target_y_offset+Height-BAR_WIDTH -PAD_WIDTH3 > target_height)
		target_y_offset  = target_height - Height+BAR_WIDTH+PAD_WIDTH3;
	      XMoveWindow(dpy,target,-target_x_offset, -target_y_offset);
	      motion = NONE;
	      exposed = 2;
	    }
	  if(motion == VERTICAL)
	    {
	      target_y_offset=(Event.xbutton.y-PAD_WIDTH3-SCROLL_BAR_WIDTH)*target_height/
		(Height-BAR_WIDTH-PAD_WIDTH3 - 2*SCROLL_BAR_WIDTH);
	      if(target_y_offset+Height-BAR_WIDTH -PAD_WIDTH3 > target_height)
		target_y_offset  = target_height - Height+BAR_WIDTH+PAD_WIDTH3;
	      if(target_y_offset < 0)
		target_y_offset = 0;
	      XMoveWindow(dpy,target,-target_x_offset, -target_y_offset);
	    }
	  if(motion == HORIZONTAL)
	    {
	      target_x_offset=(Event.xbutton.x -PAD_WIDTH3-SCROLL_BAR_WIDTH)* target_width/
		(Width-BAR_WIDTH-PAD_WIDTH3-2*SCROLL_BAR_WIDTH);
	      if(target_x_offset < 0)
		target_x_offset = 0;
	      
	      if(target_x_offset + Width - BAR_WIDTH -PAD_WIDTH3> target_width)
		target_x_offset  = target_width - Width + BAR_WIDTH+PAD_WIDTH3;
	      XMoveWindow(dpy,target,-target_x_offset, -target_y_offset);
	    }
	  RedrawWindow(target);
	  motion = NONE;
	  break;

	case MotionNotify:
	  if((motion == LEFT)&&((Event.xmotion.y < Height-BAR_WIDTH) ||
	     (Event.xmotion.x > SCROLL_BAR_WIDTH+PAD_WIDTH3)))
	    {
	      motion = NONE;
	      exposed = 2;
	    }
	  else if((motion == RIGHT)&&((Event.xmotion.y < Height-BAR_WIDTH) ||
	     (Event.xmotion.x < Width-BAR_WIDTH-SCROLL_BAR_WIDTH-2) ||
	     (Event.xmotion.x > Width-BAR_WIDTH)))
	    {
	      motion = NONE;
	      exposed = 2;
	    }
	  else if((motion == TOP)&&
		  ((Event.xmotion.y>SCROLL_BAR_WIDTH+PAD_WIDTH3)||
		   (Event.xmotion.x < Width-BAR_WIDTH)))
	    {
	      motion = NONE;
	      exposed = 2;
	    }
	  else if((motion == BOTTOM)&&
		  ((Event.xmotion.y < Height-BAR_WIDTH-SCROLL_BAR_WIDTH-2) ||
		   (Event.xmotion.y > Height-BAR_WIDTH)||
		   (Event.xmotion.x < Width-BAR_WIDTH)))
	    {
	      motion = NONE;
	      exposed = 2;
	    }
	  if(motion == VERTICAL)
	    {
	      target_y_offset=(Event.xmotion.y-PAD_WIDTH3-SCROLL_BAR_WIDTH)*
		target_height/
		(Height-BAR_WIDTH-PAD_WIDTH3-2*SCROLL_BAR_WIDTH);
	      if(target_y_offset+Height-BAR_WIDTH -PAD_WIDTH3 > target_height)
		target_y_offset  = target_height - Height+BAR_WIDTH+PAD_WIDTH3;
	      if(target_y_offset < 0)
		target_y_offset = 0;
	      XMoveWindow(dpy,target,-target_x_offset, -target_y_offset);
	    }
	  if(motion == HORIZONTAL)
	    {
	      target_x_offset=(Event.xmotion.x -PAD_WIDTH3-SCROLL_BAR_WIDTH)*
		target_width/
		(Width-BAR_WIDTH-PAD_WIDTH3-2*SCROLL_BAR_WIDTH);
	      if(target_x_offset < 0)
		target_x_offset = 0;
	      
	      if(target_x_offset + Width - BAR_WIDTH -PAD_WIDTH3> target_width)
		target_x_offset  = target_width - Width + BAR_WIDTH+PAD_WIDTH3;
	      XMoveWindow(dpy,target,-target_x_offset, -target_y_offset);
	    }
	  if((motion == QUIT)&&
	    ((Event.xbutton.y < Height- BAR_WIDTH )||
	       (Event.xbutton.x < Width- BAR_WIDTH)))
	    {
	      motion = NONE;
	      exposed = 2;
	    }
	  RedrawWindow(target);
	  break;
	case ClientMessage:
	  if ((Event.xclient.format==32) && 
	      (Event.xclient.data.l[0]==wm_del_win))
	    {
	      DeadPipe(1);
	    }
	  break;
	case PropertyNotify:
	  if(Event.xproperty.atom == XA_WM_NAME)
	    {
	      if(XFetchName(dpy, target, &temp)==0)
		temp = NULL;
	      change_window_name(temp);
	    }
	  else if (Event.xproperty.atom == XA_WM_ICON_NAME)
	    {
	      if (XGetWindowProperty (dpy,
				      target, Event.xproperty.atom, 0,
				      MAX_ICON_NAME_LEN, False, XA_STRING, 
				      &actual,&actual_format, &nitems, 
				      &bytesafter, (unsigned char **) &prop)
		  == Success && (prop != NULL))
		change_icon_name(prop);
	    }
	  else if(Event.xproperty.atom == XA_WM_HINTS)
	    {
	      XWMHints *wmhints;

	      wmhints = XGetWMHints(dpy,target);
	      XSetWMHints(dpy,main_win, wmhints);
	      XFree(wmhints);
	    }
	  else if(Event.xproperty.atom == XA_WM_NORMAL_HINTS)	  
	    {
	      /* don't do Normal Hints. They alter the size of the window */
	    }
	  else if (Event.xproperty.atom == _XA_WM_COLORMAP_WINDOWS)
	    {
	      /* ignore colormap stuff. The FvwmScroll window may have a
	       * different visual to the client and so cannot pass this on */
	    }
	  break;
	  
	case DestroyNotify:
	  DeadPipe(1);
	  break;

	case UnmapNotify:
	  break;

	case MapNotify:
	  XMapWindow(dpy,main_win);
	  break;
	case FocusIn:
	  XSetInputFocus(dpy,target,RevertToParent,CurrentTime);
	  break;
	case ColormapNotify:
	/* FvwmScroll may not have the same visual as the captured window
	 * and so cannot share its colormap
	  {
	    XWindowAttributes xwa;
	    if(XGetWindowAttributes(dpy,target, &xwa) != 0)
	      {
		XSetWindowColormap(dpy,main_win,xwa.colormap);
	      }
	  }
	*/
	  break;
	default:
	  break;
	}
    }

    /* no more events to process */
    /* wait for X-event or config line */
    select( fd_width, SELECT_FD_SET_CAST &fdset, NULL, NULL, NULL );

    /* parse any config lines (only the fvwm_look) */
    if (FD_ISSET(fd[1], &fdset)) {
      char *tline;

      GetConfigLine(fd, &tline);
      if (tline != NULL && (strlen(tline) > 1)) {
        if(strncasecmp(tline, DEFGRAPHSTR, DEFGRAPHLEN)==0) {
          if (ParseGraphics(dpy, tline, G)) {
            SetWindowBackground(dpy, main_win, tw, th, G->bg, G->shadowGC);
          }
        }
      }

      /* free up line malloc'd by GetConfigLine */
      if (tline) free(tline);
    }
  }
  return;
}



/************************************************************************
 *
 * Draw the window 
 *
 ***********************************************************************/
void RedrawWindow(Window target)
{
  static int xv= 0,yv= 0,hv=0,wv=0;
  static int xh=0,yh=0,hh=0,wh=0;
  int x,y,w,h;
  XEvent dummy;

  while (XCheckTypedWindowEvent (dpy, main_win, Expose, &dummy))
    exposed |= 1;
  
  XSetWindowBorderWidth(dpy,target,0);

  RelieveRectangle(dpy, main_win, PAD_WIDTH3 - 2, PAD_WIDTH3 - 2,
		   Width-BAR_WIDTH - PAD_WIDTH3 + 3,
		   Height-BAR_WIDTH - PAD_WIDTH3 + 3,
		   G->shadowGC, G->reliefGC, 2);

  y = (Height-BAR_WIDTH-PAD_WIDTH3-2*SCROLL_BAR_WIDTH)*
    target_y_offset/target_height
      + PAD_WIDTH2 + 2 + SCROLL_BAR_WIDTH;
  x = Width-SCROLL_BAR_WIDTH- PAD_WIDTH2-2;
  w = SCROLL_BAR_WIDTH;
  h = (Height-BAR_WIDTH-PAD_WIDTH3-2*SCROLL_BAR_WIDTH)*
    (Height-BAR_WIDTH-PAD_WIDTH3)/
      target_height;
  if((y!=yv)||(x != xv)||(w != wv)||(h != hv)||(exposed & 1))
    {
      yv = y;
      xv = x;
      wv = w;
      hv = h;
      XClearArea(dpy,main_win,x,PAD_WIDTH3+SCROLL_BAR_WIDTH,
		 w,Height-BAR_WIDTH-PAD_WIDTH3-2*SCROLL_BAR_WIDTH,False);

      RelieveRectangle(dpy, main_win, x, y, w - 1, h - 1,
		       G->reliefGC, G->shadowGC, 2);
    }
  if(exposed & 1)
      RelieveRectangle(dpy, main_win, x - 2, PAD_WIDTH2, w + 3,
		       Height - BAR_WIDTH - PAD_WIDTH2 + 1,
		       G->shadowGC, G->reliefGC, 2);
  if(exposed)
    {
      if(motion == TOP)
	RedrawTopButton(G->shadowGC,G->reliefGC,x,PAD_WIDTH3);
      else
	RedrawTopButton(G->reliefGC,G->shadowGC,x,PAD_WIDTH3);
      if(motion == BOTTOM)
	RedrawBottomButton(G->shadowGC,G->reliefGC,x,
			   Height-BAR_WIDTH-SCROLL_BAR_WIDTH);
      else
	RedrawBottomButton(G->reliefGC,G->shadowGC,x,
			   Height-BAR_WIDTH-SCROLL_BAR_WIDTH);
    }

  x = (Width-BAR_WIDTH-PAD_WIDTH3-2*SCROLL_BAR_WIDTH)*target_x_offset/
    target_width+PAD_WIDTH2+2 + SCROLL_BAR_WIDTH;
  y = Height-SCROLL_BAR_WIDTH-PAD_WIDTH2-2;
  w = (Width-BAR_WIDTH-PAD_WIDTH3-2*SCROLL_BAR_WIDTH)*
    (Width-BAR_WIDTH-PAD_WIDTH3)/target_width;
  h = SCROLL_BAR_WIDTH;
  if((y!=yh)||(x != xh)||(w != wh)||(h != hh)||(exposed & 1))
    {
      yh = y;
      xh = x;
      wh = w;
      hh = h;
      XClearArea(dpy,main_win,PAD_WIDTH3+SCROLL_BAR_WIDTH,y,
		 Width-BAR_WIDTH-PAD_WIDTH3-2*SCROLL_BAR_WIDTH,h,False);
      RelieveRectangle(dpy, main_win, x, y, w - 1, h - 1,
		       G->reliefGC, G->shadowGC, 2);
    }
  if(exposed& 1)
    {
      RelieveRectangle(dpy, main_win, PAD_WIDTH2, y - 2,
		       Width - BAR_WIDTH - PAD_WIDTH2 + 1, h + 2,
		       G->shadowGC, G->reliefGC, 2);
    }
  if(exposed)
    {
      if(motion == LEFT)
	RedrawLeftButton(G->shadowGC,G->reliefGC,PAD_WIDTH3,y);
      else
	RedrawLeftButton(G->reliefGC,G->shadowGC,PAD_WIDTH3,y);
      if(motion ==RIGHT)
	RedrawRightButton(G->shadowGC,G->reliefGC,
			  Width-BAR_WIDTH-SCROLL_BAR_WIDTH,y);
      else
	RedrawRightButton(G->reliefGC,G->shadowGC,
			  Width-BAR_WIDTH-SCROLL_BAR_WIDTH,y);
    }

  if(exposed)
    {
      XClearArea(dpy,main_win,Width-BAR_WIDTH+2,
		     Height-BAR_WIDTH+2,BAR_WIDTH-3,BAR_WIDTH-3,False);
      if(motion == QUIT)
      RelieveRectangle(dpy, main_win, Width - SCROLL_BAR_WIDTH - PAD_WIDTH2 - 4,
		       Height - SCROLL_BAR_WIDTH - PAD_WIDTH2 - 4,
		       SCROLL_BAR_WIDTH + 3, SCROLL_BAR_WIDTH + 3,
		       G->shadowGC, G->reliefGC, 2);
      else
	RelieveRectangle(dpy, main_win, Width - SCROLL_BAR_WIDTH - PAD_WIDTH2 - 4,
		         Height - SCROLL_BAR_WIDTH - PAD_WIDTH2 - 4,
		         SCROLL_BAR_WIDTH + 3,SCROLL_BAR_WIDTH + 3,
		         G->reliefGC, G->shadowGC, 2);
    }
  exposed = 0;
}


/**************************************************************************
 *  Change the window name displayed in the title bar.
 **************************************************************************/
void change_window_name(char *str)
{
  XTextProperty name;
  
  if(str == NULL)
    return;

  if (XStringListToTextProperty(&str,1,&name) == 0) 
    {
      fprintf(stderr,"%s: cannot allocate window name",MyName);
      return;
    }
  XSetWMName(dpy,main_win,&name);
  XFree(name.value);
}


/**************************************************************************
 *  Change the window name displayed in the icon.
 **************************************************************************/
void change_icon_name(char *str)
{
  XTextProperty name;
  
  if(str == NULL)return;
  if (XStringListToTextProperty(&str,1,&name) == 0) 
    {
      fprintf(stderr,"%s: cannot allocate window name",MyName);
      return;
    }
  XSetWMIconName(dpy,main_win,&name);
  XFree(name.value);
}


void GrabWindow(Window target)
{
  char *temp;
  Window Junk,root;
  unsigned int tw,th,border_width,depth;
  int x,y;
  char *prop = NULL;
  Atom actual = None;
  int actual_format;
  unsigned long nitems, bytesafter;
     
  XUnmapWindow(dpy,target);
  XSync(dpy,0);
  XGetGeometry(dpy,target,&root,&x,&y,
	       (unsigned int *)&tw,(unsigned int *)&th,
	       (unsigned int *)&border_width,
	       (unsigned int *)&depth);
  XSync(dpy,0);

  XTranslateCoordinates(dpy, target, Root, 0, 0, &x,&y, &Junk);

  CreateWindow(x,y,tw,th);
  XSetWindowBorderWidth(dpy,target,0);
  XReparentWindow(dpy,target, holder_win,0,0);
  XMapWindow(dpy,target);
  XSelectInput(dpy,target, PropertyChangeMask|StructureNotifyMask);
  if(XFetchName(dpy, target, &temp)==0)
    temp = NULL;
  if (XGetWindowProperty (dpy,
			  target, XA_WM_ICON_NAME, 0,
			  MAX_ICON_NAME_LEN, False, XA_STRING, 
			  &actual,&actual_format, &nitems, 
			  &bytesafter, (unsigned char **) &prop)
      == Success && (prop != NULL))
    {
      change_icon_name(prop);
      XFree(prop);
    }
  change_window_name(temp);
  {
    XWMHints *wmhints;
    
    wmhints = XGetWMHints(dpy,target);
    if(wmhints != NULL)
      {
	XSetWMHints(dpy,main_win, wmhints);
	XFree(wmhints);
      }
  }
/* FvwmScroll may not share the same visual as the target window
 * and so cannot share its colormap  {
    XWindowAttributes xwa;
    if(XGetWindowAttributes(dpy,target, &xwa) != 0)
      {
	XSetWindowColormap(dpy,main_win,xwa.colormap);
      }
  }
*/    
  XMapWindow(dpy,main_win);
  RedrawWindow(target);
  XFree(temp);
}





void RedrawLeftButton(GC rgc, GC sgc,int x1,int y1)
{
  XSegment seg[4];  
  int i=0;
  
  seg[i].x1 = x1+1;		        seg[i].y1   = y1+SCROLL_BAR_WIDTH/2;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 2;	seg[i++].y2 = y1+1;

  seg[i].x1 = x1;		        seg[i].y1   = y1+SCROLL_BAR_WIDTH/2;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 1;	seg[i++].y2 = y1;
  XDrawSegments(dpy, main_win, rgc, seg, i);

  i = 0;
  seg[i].x1 = x1+1;		        seg[i].y1   =y1+ SCROLL_BAR_WIDTH/2;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 2;	seg[i++].y2 =y1+ SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1;		        seg[i].y1   = y1+SCROLL_BAR_WIDTH/2;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 1;	seg[i++].y2 = y1+SCROLL_BAR_WIDTH - 1;

  seg[i].x1 = x1+SCROLL_BAR_WIDTH - 2;	seg[i].y1   = y1+1;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 2;	seg[i++].y2 = y1+SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1+SCROLL_BAR_WIDTH - 1;	seg[i].y1   = y1;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 1;	seg[i++].y2 = y1+SCROLL_BAR_WIDTH - 1;
  XDrawSegments(dpy,main_win, sgc, seg, i);
}

void RedrawRightButton(GC rgc, GC sgc,int x1,int y1)
{
  XSegment seg[4];  
  int i=0;

  seg[i].x1 = x1+1;		seg[i].y1   = y1+1;
  seg[i].x2 = x1+1;		seg[i++].y2 = y1+SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1;		seg[i].y1 = y1;
  seg[i].x2 = x1;		seg[i++].y2 = y1+SCROLL_BAR_WIDTH - 1;

  seg[i].x1 = x1+1;		seg[i].y1 = y1+1;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 2;	seg[i++].y2 = y1+SCROLL_BAR_WIDTH/2;

  seg[i].x1 = x1;		seg[i].y1 = y1;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 1;	seg[i++].y2 = y1+SCROLL_BAR_WIDTH/2;

  XDrawSegments(dpy, main_win, rgc, seg, i);

  i = 0;
  seg[i].x1 = x1;		seg[i].y1 = y1+SCROLL_BAR_WIDTH - 2;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 2;	seg[i++].y2 = y1+SCROLL_BAR_WIDTH/2;

  seg[i].x1 = x1;		        seg[i].y1 = y1+SCROLL_BAR_WIDTH - 1;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 1;	seg[i++].y2 = y1+SCROLL_BAR_WIDTH/2;
  XDrawSegments(dpy,main_win, sgc, seg, i);
}

void RedrawTopButton(GC rgc, GC sgc,int x1,int y1)
{
  XSegment seg[4];  
  int i=0;

  seg[i].x1 = x1+SCROLL_BAR_WIDTH/2;	seg[i].y1 = y1+1;
  seg[i].x2 = x1+1;		seg[i++].y2 = y1+SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1+SCROLL_BAR_WIDTH/2;	seg[i].y1 = y1;
  seg[i].x2 = x1+0;		seg[i++].y2 = y1+SCROLL_BAR_WIDTH - 1;
  XDrawSegments(dpy, main_win, rgc, seg, i);

  i = 0;
  seg[i].x1 = x1+SCROLL_BAR_WIDTH/2;	seg[i].y1 = y1+1;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 2;	seg[i++].y2 = y1+SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1+SCROLL_BAR_WIDTH/2;	seg[i].y1 = y1;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 1;	seg[i++].y2 = y1+SCROLL_BAR_WIDTH - 1;

  seg[i].x1 = x1+1;		seg[i].y1 = y1+SCROLL_BAR_WIDTH - 2;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 2;	seg[i++].y2 = y1+SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1+0;		seg[i].y1 = y1+SCROLL_BAR_WIDTH - 1;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 1;	seg[i++].y2 = y1+SCROLL_BAR_WIDTH - 1;
  XDrawSegments(dpy,main_win, sgc, seg, i);
}

void RedrawBottomButton(GC rgc, GC sgc,int x1, int y1)
{
  XSegment seg[4];  
  int i=0;

  seg[i].x1 = x1+1;		seg[i].y1 = y1+1;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH/2;	seg[i++].y2 = y1+SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1;		seg[i].y1 = y1+0;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH/2;	seg[i++].y2 = y1+SCROLL_BAR_WIDTH - 1;

  seg[i].x1 = x1+1;		seg[i].y1 = y1+1;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 2;	seg[i++].y2 = y1+1;

  seg[i].x1 = x1;		seg[i].y1 = y1+0;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH - 1;	seg[i++].y2 = y1+0;
  XDrawSegments(dpy,main_win, rgc, seg, i);

  i = 0;
  seg[i].x1 = x1+SCROLL_BAR_WIDTH - 2;	seg[i].y1 = y1+1;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH/2;	seg[i++].y2 = y1+SCROLL_BAR_WIDTH - 2;

  seg[i].x1 = x1+SCROLL_BAR_WIDTH - 1;	seg[i].y1 = y1+0;
  seg[i].x2 = x1+SCROLL_BAR_WIDTH/2;	seg[i++].y2 = y1+SCROLL_BAR_WIDTH - 1;
  XDrawSegments(dpy, main_win, sgc, seg, i);
}
  
