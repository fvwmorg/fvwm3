/****************************************************************************
 * This module is all new
 * by Rob Nation 
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 ****************************************************************************/


#include "../configure.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>

#include <X11/Xproto.h>
#include <X11/Xatom.h>


#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "../version.h"

char *white = "white";
char *black = "black";

extern char *Hiback;
extern char *Hifore;

/****************************************************************************
 *
 * This routine computes the shadow color from the background color
 *
 ****************************************************************************/
Pixel GetShadow(Pixel background) 
{
  XColor bg_color;
  XWindowAttributes attributes;
  unsigned int r,g,b;
  
  XGetWindowAttributes(dpy,Scr.Root,&attributes);
  
  bg_color.pixel = background;
  XQueryColor(dpy,attributes.colormap,&bg_color);
  
  r = bg_color.red % 0xffff;
  g = bg_color.green % 0xffff;
  b = bg_color.blue % 0xffff;
  
  r = r >>1;
  g = g >>1;
  b = b >>1;
  
  bg_color.red = r;
  bg_color.green = g;
  bg_color.blue = b;

  if(!XAllocColor(dpy,attributes.colormap,&bg_color))
    {
      nocolor("alloc shadow","");
      bg_color.pixel = background;
    }
  
  return bg_color.pixel;
}

/****************************************************************************
 *
 * This routine computes the hilight color from the background color
 *
 ****************************************************************************/
Pixel GetHilite(Pixel background) 
{
  XColor bg_color, white_p;
  XWindowAttributes attributes;
  
  XGetWindowAttributes(dpy,Scr.Root,&attributes);
  
  bg_color.pixel = background;
  XQueryColor(dpy,attributes.colormap,&bg_color);

  white_p.pixel = GetColor(white);
  XQueryColor(dpy,attributes.colormap,&white_p);
  
#ifndef min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#define max(a,b) (((a)>(b)) ? (a) : (b))
#endif

  bg_color.red = max((white_p.red/5), bg_color.red);
  bg_color.green = max((white_p.green/5), bg_color.green);
  bg_color.blue = max((white_p.blue/5), bg_color.blue);
  
  bg_color.red = min(white_p.red, (bg_color.red*140)/100);
  bg_color.green = min(white_p.green, (bg_color.green*140)/100);
  bg_color.blue = min(white_p.blue, (bg_color.blue*140)/100);

#undef min
#ifdef max
#undef max
#endif
  
  if(!XAllocColor(dpy,attributes.colormap,&bg_color))
    {
      nocolor("alloc hilight","");
      bg_color.pixel = background;
    }
  return bg_color.pixel;
}

/***********************************************************************
 *
 *  Procedure:
 *	CreateGCs - open fonts and create all the needed GC's.  I only
 *		    want to do this once, hence the first_time flag.
 *
 ***********************************************************************/
void CreateGCs(void)
{
  XGCValues gcv;
  unsigned long gcm;
  
  /* create scratch GC's */
  gcm = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth;
  gcv.line_width = 0;
  gcv.function = GXcopy;
  gcv.plane_mask = AllPlanes;
  gcv.graphics_exposures = False;
  
  Scr.ScratchGC1 = XCreateGC(dpy, Scr.Root, gcm, &gcv);
  Scr.ScratchGC2 = XCreateGC(dpy, Scr.Root, gcm, &gcv);
  Scr.ScratchGC3 = XCreateGC(dpy, Scr.Root, gcm, &gcv);

#if defined(PIXMAP_BUTTONS) || defined(GRADIENT_BUTTONS)
  Scr.TransMaskGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);
#endif
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
  if (!XParseColor (dpy, Scr.FvwmRoot.attr.colormap, name, &color)) 
    {
      nocolor("parse",name);
    }
  else if(!XAllocColor (dpy, Scr.FvwmRoot.attr.colormap, &color)) 
    {
      nocolor("alloc",name);
    }
  return color.pixel;
}

#ifdef GRADIENT_BUTTONS
/****************************************************************************
 * 
 * Allocates a nonlinear color gradient (veliaa@rpi.edu)
 *
 ****************************************************************************/ 
Pixel *AllocNonlinearGradient(char *s_colors[], int clen[],
			      int nsegs, int npixels)
{
    Pixel *pixels = (Pixel *)safemalloc(sizeof(Pixel) * npixels);
    int i = 0, curpixel = 0, perc = 0;
    if (nsegs < 1) {
	fvwm_msg(ERR,"AllocNonlinearGradient",
		 "must specify at least one segment");
	free(pixels);
	return NULL;
    }
    for (; i < npixels; i++)
	pixels[i] = 0;

    for (i = 0; (i < nsegs) && (curpixel < npixels) && (perc <= 100); ++i) {
	Pixel *p;
	int j = 0, n = clen[i] * npixels / 100;
	p = AllocLinearGradient(s_colors[i], s_colors[i + 1], n);
	if (!p) {
	    fvwm_msg(ERR,"AllocNonlinearGradient",
		     "couldn't allocate gradient");
	    free(pixels);
	    return NULL;
	}
	for (; j < n; ++j)
	    pixels[curpixel + j] = p[j];	
	perc += clen[i];
	curpixel += n;
	free(p);
    }
    for (i = curpixel; i < npixels; ++i)
      pixels[i] = pixels[i - 1];
    return pixels;
}

/****************************************************************************
 * 
 * Allocates a linear color gradient (veliaa@rpi.edu)
 *
 ****************************************************************************/ 
Pixel *AllocLinearGradient(char *s_from, char *s_to, int npixels)
{
    Pixel *pixels;
    XColor from, to, c;
    int r, dr, g, dg, b, db;
    int i = 0, got_all = 1;
    
    if (npixels < 1) return NULL;
    if (!s_from || !XParseColor(dpy, Scr.FvwmRoot.attr.colormap, s_from, &from)) {
	nocolor("parse", s_from);
	return NULL;
    }
    if (!s_to || !XParseColor(dpy, Scr.FvwmRoot.attr.colormap, s_to, &to)) {
	nocolor("parse", s_to);
	return NULL;
    }
    c = from;
    r = from.red; dr = (to.red - from.red) / npixels;
    g = from.green; dg = (to.green - from.green) / npixels;
    b = from.blue; db = (to.blue - from.blue) / npixels;
    pixels = (Pixel *)safemalloc(sizeof(Pixel) * npixels);
    c.flags = DoRed | DoGreen | DoBlue;
    for (; i < npixels; ++i)
    {
	if (!XAllocColor(dpy, Scr.FvwmRoot.attr.colormap, &c))
	    got_all = 0;
	pixels[ i ] = c.pixel;
	c.red = (unsigned short) (r += dr);
	c.green = (unsigned short) (g += dg);
	c.blue = (unsigned short) (b += db);
    }
    if (!got_all) {
	char s[256];
	sprintf(s, "color gradient %s to %s", s_from, s_to);
	nocolor("alloc", s);
    }
    return pixels;
}
#endif /* GRADIENT_BUTTONS */

void nocolor(char *note, char *name)
{
  fvwm_msg(ERR,"nocolor","can't %s color %s", note,name);
}
