/*
   Fvwmbuttons v2.0.41-plural-Z-alpha, copyright 1996, Jarl Totland

 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. 

*/
/***********************************************************************
 *
 * Derived from fvwm icon code
 *
 ***********************************************************************/

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "../../libs/fvwmlib.h"
#include "FvwmButtons.h"

#ifdef XPM
#include <X11/xpm.h>
#endif /* XPM */

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

/****************************************************************************
 *
 * Creates an Icon Window
 *
 ****************************************************************************/
void CreateIconWindow(button_info *b)
{
#ifndef NO_ICONS
  unsigned long valuemask;		/* mask for create windows */
  XSetWindowAttributes attributes;	/* attributes for create windows */

  if(!(b->flags&b_Icon))
    return;

  if(b->IconWin != None)
    {
      fprintf(stderr,"%s: BUG: Iconwindow already created for 0x%lx!\n",
	      MyName,(unsigned long)b);
      exit(2);
    }

  attributes.background_pixel = buttonBack(b);
  attributes.event_mask = ExposureMask;
  valuemask =  CWEventMask | CWBackPixel;

  if(b->icon->width<1 || b->icon->height<1)
    {
      fprintf(stderr,"%s: BUG: Illegal iconwindow tried created\n",MyName);
      exit(2);
    }
  b->IconWin=XCreateWindow(Dpy,MyWindow,0,0,b->icon->width,b->icon->height,
			   0, CopyFromParent, CopyFromParent,CopyFromParent,
			   valuemask,&attributes);

#ifdef XPM
#ifdef SHAPE
  if (b->icon->mask!=None)
    XShapeCombineMask(Dpy,b->IconWin,ShapeBounding,0,0,
		      b->icon->mask,ShapeSet);
#endif
#endif

  if(b->icon->depth==0)
    {
      XGCValues gcv;
      unsigned long gcm=0;
      Pixmap temp;
  
      gcm = GCForeground | GCBackground;
      gcv.background=buttonBack(b);
      gcv.foreground=buttonFore(b);
      XChangeGC(Dpy,NormalGC,gcm,&gcv);
  
#ifdef SHAPE
      XShapeCombineMask(Dpy,b->IconWin,ShapeBounding,0,0,
			b->icon->picture,ShapeSet);
#endif
  
      temp = XCreatePixmap(Dpy,Root,b->icon->width,
			   b->icon->height,d_depth);
      XCopyPlane(Dpy,b->icon->picture,temp,NormalGC,
		 0,0,b->icon->width,b->icon->height,0,0,1);
      
      XSetWindowBackgroundPixmap(Dpy,b->IconWin,temp);
      XFreePixmap(Dpy,temp);
      /* We won't use the icon pixmap anymore... but we still need it for
	 width/height etc. so we can't destroy it. */
    }
  else
    XSetWindowBackgroundPixmap(Dpy,b->IconWin,b->icon->picture);

  return;
#endif
}


/****************************************************************************
 *
 * Combines icon shape masks after a resize
 *
 ****************************************************************************/
void ConfigureIconWindow(button_info *b)
{
#ifndef NO_ICONS
  int x,y,w,h;
  int xoff,yoff;
  int framew,xpad,ypad;
  XFontStruct *font;
  int BW,BH;

  if(!b || !(b->flags&b_Icon))
    return;

  if(!b->IconWin)
    {
      fprintf(stderr,"%s: DEBUG: Tried to configure erroneous iconwindow\n",
	      MyName);
      exit(2);
    }

  buttonInfo(b,&x,&y,&xpad,&ypad,&framew);
  framew=abs(framew);

  font = buttonFont(b);
  w = b->icon->width;
  h = b->icon->height;
  BW = buttonWidth(b);
  BH = buttonHeight(b);

  w=min(w,BW-2*(xpad+framew));

  if(b->flags&b_Title && font && !(buttonJustify(b)&b_Horizontal))
    h=min(h,BH-2*(ypad+framew)-font->ascent-font->descent);
  else
    h=min(h,BH-2*(ypad+framew));

  if(w < 1 || h < 1)
    {
      XMoveResizeWindow(Dpy, b->IconWin, 2000,2000,1,1);
      return; /* No need drawing to this */
    }

  if(buttonJustify(b)&b_Horizontal)
    xoff=0;
  else
    xoff=(BW-w)>>1;

  if(b->flags&b_Title && font && !(buttonJustify(b)&b_Horizontal))
    yoff=(BH-(h+font->ascent+font->descent))>>1;
  else
    yoff=(BH-h)>>1;
  
  if(xoff < framew+xpad)
    xoff = framew+xpad;
  if(yoff < framew+ypad)
    yoff = framew+ypad;

  x += xoff;
  y += yoff;

  XMoveResizeWindow(Dpy, b->IconWin, x,y,w,h);

/* Doesn't this belong above? 
#ifdef XPM
#ifdef SHAPE
  if (b->icon->mask!=None)
    {
      XShapeCombineMask(Dpy,b->IconWin,ShapeBounding,0,0,
			b->icon->mask,ShapeSet);
    }
#endif
#endif
  if(b->icon->depth==0)
    {
      PixmapFromBitmap(b);
    }
  XSetWindowBackgroundPixmap(Dpy,b->IconWin,b->icon->picture);
*/

#endif 
}
