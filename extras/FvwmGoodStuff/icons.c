/* This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */
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

#ifdef HAVE_FNCTL_H
#include <fcntl.h>
#endif

#include "FvwmGoodStuff.h"

#ifdef XPM
#include <X11/xpm.h>
#endif /* XPM */
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

extern int xpad;
extern int ypad;
extern int framew;

/****************************************************************************
 *
 * Loads an icon file into a pixmap
 *
 ****************************************************************************/
void LoadIconFile(int button)
{
#ifndef NO_ICONS
  /* First, check for a monochrome bitmap */
  if(Buttons[button].icon_file != NULL)
    GetBitmapFile(button);

  /* Next, check for a color pixmap */
  if((Buttons[button].icon_file != NULL)&&
     (Buttons[button].icon_w == 0)&&(Buttons[button].icon_h == 0))
    GetXPMFile(button);
#endif
}

/****************************************************************************
 *
 * Creates an Icon Window
 *
 ****************************************************************************/
void CreateIconWindow(int button)
{
#ifndef NO_ICONS
  unsigned long valuemask;		/* mask for create windows */
  XSetWindowAttributes attributes;	/* attributes for create windows */

  if((Buttons[button].icon_w == 0)&&(Buttons[button].icon_h == 0))
    return;

  attributes.background_pixel = back_pix;
  attributes.event_mask = ExposureMask;
  valuemask =  CWEventMask | CWBackPixel;

  Buttons[button].IconWin = 
    XCreateWindow(dpy, main_win, 0, 0, Buttons[button].icon_w,
		  Buttons[button].icon_w, 0, CopyFromParent,
		  CopyFromParent,CopyFromParent,valuemask,&attributes);
  return;
#endif
}

/****************************************************************************
 *
 * Combines icon shape masks after a resize
 *
 ****************************************************************************/
void ConfigureIconWindow(int button,int row, int column)
{
#ifndef NO_ICONS
  int x,y,w,h;
  int xoff,yoff;
  Pixmap temp;

  if((Buttons[button].icon_w == 0)&&(Buttons[button].icon_h == 0))
    return;
  
  if(Buttons[button].swallow != 0)
    return;

  w = Buttons[button].icon_w;
  h = Buttons[button].icon_h;
  if(w > Buttons[button].BWidth*ButtonWidth - 2*(xpad+framew))
    w = Buttons[button].BWidth*ButtonWidth - 2*(xpad+framew);
  if(strcmp(Buttons[button].title,"-")==0 || font==NULL)
    {
      if(h>Buttons[button].BHeight*ButtonHeight-2*(ypad+framew))
	h= Buttons[button].BHeight*ButtonHeight-2*(ypad+framew);
    }
  else
    {
      if(h>Buttons[button].BHeight*ButtonHeight-2*(ypad+framew)-font->ascent-font->descent)
	h= Buttons[button].BHeight*ButtonHeight-2*(ypad+framew)-font->ascent-font->descent;
    }

  if(w < 1)
    w = 1;
  if(h < 1)
    h = 1;

  x = column*ButtonWidth;
  y = row*ButtonHeight; 
  xoff = (Buttons[button].BWidth*ButtonWidth - w)>>1;
  if(strcmp(Buttons[button].title,"-")==0 || font==NULL)
    {
      yoff = (Buttons[button].BHeight*ButtonHeight - h)>>1;
    } 
  else
    {
      yoff = (Buttons[button].BHeight*ButtonHeight - (h + font->ascent + font->descent))>>1;
    }
  
  if(xoff < 2)
    xoff = 2;
  if(yoff < 2)
    yoff = 2;
  x += xoff;
  y += yoff;
  
  XMoveResizeWindow(dpy, Buttons[button].IconWin, x,y,w,h);

#ifdef XPM
#ifdef SHAPE
  if (Buttons[button].icon_maskPixmap != None)
    {
      xoff = (w - Buttons[button].icon_w)>>1;
      yoff = (h - Buttons[button].icon_h)>>1;
      XShapeCombineMask(dpy, Buttons[button].IconWin, ShapeBounding, 0, 0,
			Buttons[button].icon_maskPixmap, ShapeSet);
    }
#endif
#endif
  if(Buttons[button].icon_depth == -1)
    {
      temp = Buttons[button].iconPixmap;
      Buttons[button].iconPixmap = 
	XCreatePixmap(dpy,Root, Buttons[button].icon_w,
		      Buttons[button].icon_h,d_depth);
      XCopyPlane(dpy,temp,Buttons[button].iconPixmap,NormalGC,
		 0,0,Buttons[button].icon_w,Buttons[button].icon_h,
		 0,0,1);
    }
  XSetWindowBackgroundPixmap(dpy, Buttons[button].IconWin,Buttons[button].iconPixmap);
    
  XClearWindow(dpy,Buttons[button].IconWin);
#endif 
}

/***************************************************************************
 *
 * Looks for a monochrome icon bitmap file
 *
 **************************************************************************/
void GetBitmapFile(int button)
{
#ifndef NO_ICONS
  char *path = NULL;
  int HotX,HotY;

  path = findIconFile(Buttons[button].icon_file, iconPath,R_OK);
  if(path == NULL)return;

  if(XReadBitmapFile (dpy, Root,path,(unsigned int *)&Buttons[button].icon_w, 
		      (unsigned int *)&Buttons[button].icon_h,
		      &Buttons[button].iconPixmap,
		      (int *)&HotX, 
		      (int *)&HotY) != BitmapSuccess)
    {
      Buttons[button].icon_w = 0;
      Buttons[button].icon_h = 0;
    }
  else
    {
      Buttons[button].icon_depth = -1;
    }
  Buttons[button].icon_maskPixmap = None;
  free(path);
#endif
}


/****************************************************************************
 *
 * Looks for a color XPM icon file
 *
 ****************************************************************************/
void GetXPMFile(int button)
{
#ifndef NO_ICONS
#ifdef XPM
  XWindowAttributes root_attr;
  XpmAttributes xpm_attributes;
  char *path = NULL;

  path = findIconFile(Buttons[button].icon_file, pixmapPath,R_OK);
  if(path == NULL)return;  

  XGetWindowAttributes(dpy,Root,&root_attr);
  xpm_attributes.colormap = root_attr.colormap;
  xpm_attributes.valuemask = XpmSize | XpmReturnPixels|XpmColormap;
  if(XpmReadFileToPixmap(dpy, Root, path,
			 &Buttons[button].iconPixmap,
			 &Buttons[button].icon_maskPixmap, 
			 &xpm_attributes) == XpmSuccess) 
    { 
      Buttons[button].icon_w = xpm_attributes.width;
      Buttons[button].icon_h = xpm_attributes.height;
      Buttons[button].icon_depth = d_depth;
    } 
  free(path);
#endif /* XPM */
#endif
}

