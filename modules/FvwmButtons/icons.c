/*
 * Fvwmbuttons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
 */

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

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "FvwmButtons.h"

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
  {
    return;
  }

  if(b->IconWin != None)
  {
    fprintf(stderr,"%s: BUG: Icon window already created for 0x%lx!\n",
	    MyName,(unsigned long)b);
    return;
  }

  attributes.background_pixel = buttonBack(b);
  attributes.background_pixmap = None;
  attributes.event_mask = ExposureMask;
  valuemask =  CWEventMask | CWBackPixmap;

  if(b->icon->width<1 || b->icon->height<1)
  {
    fprintf(stderr,"%s: BUG: Illegal iconwindow tried created\n",MyName);
    exit(2);
  }
  b->IconWin=XCreateWindow(
    Dpy, MyWindow, 0, 0, b->icon->width, b->icon->height, 0, CopyFromParent,
    CopyFromParent,CopyFromParent, valuemask,&attributes);
  if (attributes.background_pixel != None)
  {
    XSetWindowBackground(Dpy, b->IconWin, attributes.background_pixel);
  }

  if (FShapesSupported)
  {
    if (b->icon->mask!=None)
    {
      FShapeCombineMask(
	Dpy, b->IconWin, FShapeBounding, 0, 0, b->icon->mask, FShapeSet);
    }
  }

  if(b->icon->depth != Pdepth)
  {
    /* bitmap icon */
    XGCValues gcv;
    Pixmap temp;

    gcv.background=buttonBack(b);
    gcv.foreground=buttonFore(b);
    XChangeGC(Dpy,NormalGC,GCForeground | GCBackground,&gcv);

    if (FShapesSupported)
    {
      FShapeCombineMask(
	Dpy, b->IconWin, FShapeBounding, 0, 0, b->icon->picture, FShapeSet);
    }

    temp = XCreatePixmap(Dpy,MyWindow,b->icon->width,
			 b->icon->height,Pdepth);
    XCopyPlane(Dpy,b->icon->picture,temp,NormalGC,
	       0,0,b->icon->width,b->icon->height,0,0,1);

    XSetWindowBackgroundPixmap(Dpy,b->IconWin,temp);
    XFreePixmap(Dpy,temp);
    /* We won't use the icon pixmap anymore... but we still need it for
       width/height etc. so we can't destroy it. */
  }
  else
  {
    /* pixmap icon */
    XSetWindowBackgroundPixmap(Dpy,b->IconWin,b->icon->picture);
  }

  return;
#endif
}

#if 0
void CreateBackIconWindow(button_info *b)
{
#ifndef NO_ICONS
  unsigned long valuemask;		/* mask for create windows */
  XSetWindowAttributes attributes;	/* attributes for create windows */

  if(!(b->flags&b_IconBack))
  {
    return;
  }

  if(b->IconBackWin != None)
  {
    fprintf(stderr,"%s: BUG: IconBack window already created for 0x%lx!\n",
	    MyName,(unsigned long)b);
    return;
  }

  attributes.background_pixel = buttonBack(b);
  attributes.background_pixmap = None;
  attributes.event_mask = ExposureMask;
  valuemask =  CWEventMask | CWBackPixmap;

  if(b->icon->width<1 || b->icon->height<1)
  {
    fprintf(stderr,"%s: BUG: Illegal iconwindow tried created\n",MyName);
    exit(2);
  }
  b->IconWin=XCreateWindow(
    Dpy, MyWindow, 0, 0, b->icon->width, b->icon->height, 0, CopyFromParent,
    CopyFromParent,CopyFromParent, valuemask,&attributes);
  if (attributes.background_pixel != None)
  {
    XSetWindowBackground(Dpy, b->IconWin, attributes.background_pixel);
  }

  if (FShapesSupported)
  {
    if (b->icon->mask!=None)
    {
      FShapeCombineMask(
	Dpy, b->IconWin, FShapeBounding, 0, 0, b->icon->mask, FShapeSet);
    }
  }

  if(b->icon->depth != Pdepth)
  {
    XGCValues gcv;
    Pixmap temp;

    gcv.background=buttonBack(b);
    gcv.foreground=buttonFore(b);
    XChangeGC(Dpy,NormalGC,GCForeground | GCBackground,&gcv);

    if (FShapesSupported)
    {
      FShapeCombineMask(
	Dpy, b->IconWin, FShapeBounding, 0, 0, b->icon->picture, FShapeSet);
    }

    temp = XCreatePixmap(Dpy,MyWindow,b->icon->width,
			 b->icon->height,Pdepth);
    XCopyPlane(Dpy,b->icon->picture,temp,NormalGC,
	       0,0,b->icon->width,b->icon->height,0,0,1);

    XSetWindowBackgroundPixmap(Dpy,b->IconWin,temp);
    XFreePixmap(Dpy,temp);
    /* We won't use the icon pixmap anymore... but we still need it for
       width/height etc. so we can't destroy it. */
  }
  else
  {
    XSetWindowBackgroundPixmap(Dpy,b->IconWin,b->icon->picture);
  }

  return;
#endif
}
#endif

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
  FlocaleFont *Ffont;
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

  Ffont = buttonFont(b);
  w = b->icon->width;
  h = b->icon->height;
  BW = buttonWidth(b);
  BH = buttonHeight(b);

  w=min(w,BW-2*(xpad+framew));

  if(b->flags&b_Title && Ffont && !(buttonJustify(b)&b_Horizontal))
    h = min(h,BH-2*(ypad+framew)-Ffont->ascent-Ffont->descent);
  else
    h = min(h,BH-2*(ypad+framew));

  if(w < 1 || h < 1)
  {
    XMoveResizeWindow(Dpy, b->IconWin, 2000,2000,1,1);
    return; /* No need drawing to this */
  }

  if (b->flags & b_Right)
    xoff = BW-framew-xpad-w;
  else if (b->flags & b_Left)
    xoff = framew+xpad;
  else
  {
    if(buttonJustify(b)&b_Horizontal)
      xoff=0;
    else
      xoff=(BW-w)>>1;
    if(xoff < framew+xpad)
      xoff = framew+xpad;
  }

  if(b->flags&b_Title && Ffont && !(buttonJustify(b)&b_Horizontal))
    yoff=(BH-(h+Ffont->height))>>1;
  else
    yoff=(BH-h)>>1;

  if(yoff < framew+ypad)
    yoff = framew+ypad;

  x += xoff;
  y += yoff;

  XMoveResizeWindow(Dpy, b->IconWin, x,y,w,h);

  /* Doesn't this belong above? */
#if 0
  if (FShapesSupported)
  {
    if (b->icon->mask!=None)
    {
      FShapeCombineMask(
	Dpy, b->IconWin, FShapeBounding, 0, 0, b->icon->mask, FShapeSet);
    }
  }
  if(b->icon->depth <= 1)
  {
    PixmapFromBitmap(b);
  }
  XSetWindowBackgroundPixmap(Dpy,b->IconWin,b->icon->picture);
#endif
#endif
}
