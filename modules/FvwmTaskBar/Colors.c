/* Part of the FvwmTaskBar Module for Fvwm. 
 *
 *  Copyright 1994, Mike Finger (mfinger@mermaid.micro.umn.edu or
 *                               Mike_Finger@atk.com)
 *
 * The author makes not guarantees or warantees, either express or
 * implied.  Feel free to use any contained here for any purpose, as long
 * and this and any other applicible copyrights are kept intact.

 * The functions in this source file were originally part of the GoodStuff
 * and FvwmIdent modules for Fvwm, so there copyrights are also included:
 *
 * Copyright 1994, Robert Nation and Nobutaka Suzuki.
 * No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

#include "config.h"
#include <stdio.h>
#include <X11/Xlib.h>
#include "Colors.h"

extern Display *dpy;
extern Window Root;
extern char *Module;

/****************************************************************************
  Loads a single color
*****************************************************************************/ 
Pixel GetColor(char *name)
{
  XColor color;
  XWindowAttributes attributes;

  XGetWindowAttributes(dpy,Root,&attributes);
  color.pixel = 0;
   if (!XParseColor (dpy, attributes.colormap, name, &color)) 
     nocolor("parse",name);
   else if(!XAllocColor (dpy, attributes.colormap, &color)) 
       nocolor("alloc",name);
  return color.pixel;
}

/****************************************************************************
  This routine computes the hilight color from the background color
*****************************************************************************/
Pixel GetHilite(Pixel background) 
{
  XColor bg_color, white_p;
  XWindowAttributes attributes;
  
  XGetWindowAttributes(dpy,Root,&attributes);
  
  bg_color.pixel = background;
  XQueryColor(dpy,attributes.colormap,&bg_color);

  white_p.pixel = GetColor("white");
  XQueryColor(dpy,attributes.colormap,&white_p);
  
  bg_color.red = max((white_p.red/5), bg_color.red);
  bg_color.green = max((white_p.green/5), bg_color.green);
  bg_color.blue = max((white_p.blue/5), bg_color.blue);
  
  bg_color.red = min(white_p.red, (bg_color.red*140)/100);
  bg_color.green = min(white_p.green, (bg_color.green*140)/100);
  bg_color.blue = min(white_p.blue, (bg_color.blue*140)/100);
  
  if(!XAllocColor(dpy,attributes.colormap,&bg_color))
    nocolor("alloc hilight","");
  
  return bg_color.pixel;
}

/****************************************************************************
  This routine computes the shadow color from the background color
*****************************************************************************/
Pixel GetShadow(Pixel background) 
{
  XColor bg_color;
  XWindowAttributes attributes;
  
  XGetWindowAttributes(dpy,Root,&attributes);
  
  bg_color.pixel = background;
  XQueryColor(dpy,attributes.colormap,&bg_color);
  
  bg_color.red = (unsigned short)((bg_color.red*60)/100); /* was 50% */
  bg_color.green = (unsigned short)((bg_color.green*60)/100);
  bg_color.blue = (unsigned short)((bg_color.blue*60)/100);
  
  if(!XAllocColor(dpy,attributes.colormap,&bg_color))
    nocolor("alloc shadow","");
  
  return bg_color.pixel;
}

void nocolor(char *a, char *b)
{
 fprintf(stderr,"%s: can't %s %s\n", Module, a, b);
}
