/* Part of the FvwmWinList Module for Fvwm. 
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


void nocolor(char *a, char *b)
{
 fprintf(stderr,"FvwmWinList: can't %s %s\n", a,b);
}
