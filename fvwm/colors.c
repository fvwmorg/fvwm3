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

/*
 * *************************************************************************
 * This module was all new
 * by Rob Nation
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 *
 * The highlight and shadow logic is now in libs/ColorUtils.c.
 * Its completely new.
 * *************************************************************************
 */


#include "config.h"

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
#include "misc.h"
#include "parse.h"
#include "screen.h"

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
  gcm = GCFunction|GCLineWidth;
  gcv.function = GXcopy;
  gcv.line_width = 0;

  Scr.ScratchGC1 = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
  Scr.ScratchGC2 = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
  Scr.ScratchGC3 = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

#if defined(PIXMAP_BUTTONS) || defined(GRADIENT_BUTTONS)
  Scr.TransMaskGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
#endif

  /* use this only for client supplied icon pixmaps when fvwm is using
   * a non default visual */
  if (!Scr.usingDefaultVisual) {
    gcm |= GCForeground | GCBackground | GCPlaneMask;
    gcv.foreground = WhitePixel(dpy, Scr.screen);
    gcv.background = BlackPixel(dpy, Scr.screen);
    gcv.plane_mask = AllPlanes;
    Scr.IconPixmapGC = XCreateGC(dpy, RootWindow(dpy, Scr.screen), gcm, &gcv);
  } else
    Scr.IconPixmapGC = NULL;

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
  if (!XParseColor (dpy, PictureCMap, name, &color))
    {
      nocolor("parse",name);
    }
  else if(!XAllocColor (dpy, PictureCMap, &color))
    {
      nocolor("alloc",name);
    }
  return color.pixel;
}

/****************************************************************************
 *
 * Free an array of colours (n colours), never free black
 *
 ****************************************************************************/
void FreeColors(Pixel *pixels, int n)
{
  int i;

  /* We don't ever free "black" - dirty hack to allow freeing colours at all */
  for (i = 0; i < n; i++)
  {
    if (pixels[i] != 0)
      XFreeColors(dpy, PictureCMap, pixels + i, 1, 0);
  }
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
	    fvwm_msg(ERR, "AllocNonlinearGradient",
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

    if (npixels < 1) {
      fvwm_msg(ERR, "AllocLinearGradient", "Invalid number of pixels: %d",
               npixels);
      return NULL;
    }
    if (!s_from || !XParseColor(dpy, PictureCMap, s_from,
				&from)) {
	nocolor("parse", s_from);
	return NULL;
    }
    if (!s_to || !XParseColor(dpy, PictureCMap, s_to, &to)) {
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
	if (!XAllocColor(dpy, PictureCMap, &c))
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


/* Create a pixmap from a gradient specifier, width and height are hints
 * that are only used for gradients that can be tiled e.g. H or V types
 * types are HVDBSCRY for Horizontal, Vertical, Diagonal, Back-diagonal, Square,
 * Circular, Radar and Yin/Yang respectively (in order of bloatiness)
 */
#define SQRT2 1.4142135623731
#define PI    3.1415926535898
Pixmap CreateGradientPixmap(Display *dpy, Drawable d, unsigned int depth, GC gc,
			    char type, char *action, unsigned int width,
			    unsigned int height, unsigned int *width_return,
			    unsigned int *height_return)
{
  static char *me = "CreateGradientPixmap";
  Pixmap pixmap;
  Pixel *pixels;
  unsigned int ncolors = 0;
  char **colors;
  int *perc, nsegs, i;
  XGCValues xgcv;

  /* translate the gradient string into an array of colors etc */
  if (0 == (ncolors = ParseGradient(action, &colors, &perc, &nsegs))) {
    fvwm_msg(ERR, me, "Can't parse gradient: %s", action);
    return None;
  }

  /* grab the colors */
  pixels = AllocNonlinearGradient(colors, perc, nsegs, ncolors);
  for (i = 0; i <= nsegs; i++)
    if (colors[i])
      free(colors[i]);
  free(colors);
  free(perc);

  if (!pixels) {
    fvwm_msg(ERR, me, "couldn't create gradient");
    return None;
  }
 
  /* grok the size to create from the type */
  type = toupper(type);
  switch (type) {
    case 'H':
      width = ncolors;
      break;
    case 'V':
      height = ncolors;
      break;
    case 'D':
    case 'B':
      /* diagonal gradients are rendered into a rectangle for which the
       * width plus the height is equal to ncolors + 1. The rectangle is square
       * when ncolors is odd and one pixel taller than wide with even numbers */
      width = (ncolors + 1) / 2;
      height = ncolors + 1 - width;
      break;
    case 'S':
      width = height = 2 * ncolors;
      break;
    case 'C':
      /* circular gradients have the last color as a pixel in each corner */
      width = height = ncolors;
      break;
    case 'R':
    case 'Y':
      /* swept types need each color to occupy at least one pixel at the edge */
      width = height = ncolors * (1.0 / PI);
      break;
    default:
      fvwm_msg(ERR, me, "%cGradient not supported", type);
      return None;
  }
  pixmap = XCreatePixmap(dpy, d, width, height, depth);
  if (pixmap == None)
    return None;

  /* set the gc style */
  xgcv.function = GXcopy;
  xgcv.plane_mask = AllPlanes;
  xgcv.foreground = pixels[0];
  xgcv.fill_style = FillSolid;
  xgcv.clip_mask = None;
  XChangeGC(dpy, gc, GCFunction | GCPlaneMask | GCForeground | GCFillStyle
		     | GCClipMask, &xgcv);
  
  /* now do the fancy drawing */
  /* draw one pixel further than expected in case line style is CapNotLast */
  switch (type) {
    case 'H':
      for (i = 0; i < ncolors; i++) {
	xgcv.foreground = pixels[i];
	XChangeGC(dpy, gc, GCForeground, &xgcv);
	XDrawLine(dpy, pixmap, gc, i, 0, i, height);
      }
      break;
    case 'V':
      for (i = 0; i < ncolors; i++) {
	xgcv.foreground = pixels[i];
	XChangeGC(dpy, gc, GCForeground, &xgcv);
	XDrawLine(dpy, pixmap, gc, 0, i, width, i);
      }
      break;
    case 'D':
      /* split into two stages for top left corner then bottom right corner */
      /* drawn in a height by height rectangle (though width may be < height) */
      for (i = 0; i < height - 1; i++) {
	xgcv.foreground = pixels[i];
	XChangeGC(dpy, gc, GCForeground, &xgcv);
	XDrawLine(dpy, pixmap, gc, 0, i, i + 1, -1);
      }
      for (i = 0; i <= ncolors - height; i++) {
	xgcv.foreground = pixels[i + height - 1];
	XChangeGC(dpy, gc, GCForeground, &xgcv);
	XDrawLine(dpy, pixmap, gc, i, height - 1, height, i - 1);
      }
      break;
    case 'B':
      /* split into two stages for bottom left corner then top right corner */
      /* drawn in a height by height rectangle (though width may be < height) */
      for (i = 0; i < height - 1; i++) {
	xgcv.foreground = pixels[i];
	XChangeGC(dpy, gc, GCForeground, &xgcv);
	XDrawLine(dpy, pixmap, gc, 0, height - 1 - i, i + 1, height);
      }
      for (i = 0; i <= ncolors - height; i++) {
	xgcv.foreground = pixels[i + height - 1];
	XChangeGC(dpy, gc, GCForeground, &xgcv);
	XDrawLine(dpy, pixmap, gc, i, 0, height, height - i);
      }
      break;
    case 'S':
      for (i = 0; i < ncolors; i++) {
	xgcv.foreground = pixels[i];
	XChangeGC(dpy, gc, GCForeground, &xgcv);
	XDrawRectangle(dpy, pixmap, gc, ncolors - 1 - i, ncolors - 1 - i,
		       2 * i + 2, 2 * i + 2);
      }
      break;
    case 'C':
      /* seem to need fat lines to avoid missing pixels */
      xgcv.line_width = 3;
      XChangeGC(dpy, gc, GCLineWidth, &xgcv);
      for (i = 0; i < ncolors; i++) {
	register double j = (1.0 / SQRT2) * (double)i;
	xgcv.foreground = pixels[i];
	XChangeGC(dpy, gc, GCForeground, &xgcv);
	XDrawArc(dpy, pixmap, gc, width / 2 - 1 - j, width / 2 - 1 - j,
		 2.0 * j + 2.0, 2.0 * j + 2.0, 0, 64 * 360);
      }
      xgcv.line_width = 0;
      XChangeGC(dpy, gc, GCLineWidth, &xgcv);
      break;
    default:
      /* placeholder function, just fills the pixmap */
      XFillRectangle(dpy, pixmap, gc, 0, 0, width, height);
      break;
  }

  /* pass back info */
  *width_return = width;
  *height_return = height;
  return pixmap;
}


/* groks a gradient string and creates arrays of colors and percentages
 * returns the number of colors asked for (No. allocated may be less due
 * to the ColorLimit command).  A return of 0 indicates an error
 */
unsigned int ParseGradient(char *gradient, char ***colors_return,
			   int **perc_return, int *nsegs_return)
{
  static char *me = "ParseGradient";
  char *item;
  unsigned int npixels;
  char **s_colors;
  int *perc;
  int nsegs, i, sum;

  /* get the number of colors specified */
  if (!(gradient = GetNextToken(gradient, &item)) || (item == NULL)) {
    fvwm_msg(ERR, me, "expected number of colors to allocate");
    if (item)
      free(item);
    return 0;
  }
  npixels = atoi(item);
  free(item);

  /* get the starting color or number of segments */
  if (!(gradient = GetNextToken(gradient, &item)) || (item == NULL)) {
    fvwm_msg(ERR, me, "incomplete gradient style");
    if (item)
      free(item);
    return 0;
  }

  if (!(isdigit(*item))) {
    /* get the end color of a simple gradient */
    s_colors = (char **)safemalloc(sizeof(char *) * 2);
    perc = (int *)safemalloc(sizeof(int));
    nsegs = 1;
    s_colors[0] = item;
    gradient = GetNextToken(gradient, &s_colors[1]);
    perc[0] = 100;
  } else {
    /* get a list of colors and percentages */
    nsegs = atoi(item);
    free(item);
    if (nsegs < 1) nsegs = 1;
    if (nsegs > 128) nsegs = 128;
    s_colors = (char **)safemalloc(sizeof(char *) * (nsegs + 1));
    perc = (int *)safemalloc(sizeof(int) * nsegs);
    for (i = 0; i <= nsegs; i++) {
      gradient = GetNextToken(gradient, &s_colors[i]);
      if (i < nsegs) {
        gradient = GetNextToken(gradient, &item);
	if (item) {
	  perc[i] = atoi(item);
	  free(item);
	} else {
	  perc[i] = 0;
	}
      }
    }
  }

  /* sanity check */
  for (i = 0, sum = 0; i < nsegs; ++i)
    sum += perc[i];

  if (sum != 100) {
    fvwm_msg(ERR, me, "multi gradient lengths must sum to 100");
    for (i = 0; i <= nsegs; ++i)
    if (s_colors[i])
      free(s_colors[i]);
    free(s_colors);
    free(perc);
    return 0;
  }

  /* sensible limits */
  if (npixels < 2) npixels = 2;
  if (npixels > 128) npixels = 128;

  /* send data back */
  *colors_return = s_colors;
  *perc_return = perc;
  *nsegs_return = nsegs;
  return npixels;
}

#endif /* GRADIENT_BUTTONS */


void nocolor(char *note, char *name)
{
  fvwm_msg(ERR,"nocolor","can't %s color %s", note, name ? name : "<NONE>");
}
