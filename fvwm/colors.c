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
#endif /* GRADIENT_BUTTONS */

void nocolor(char *note, char *name)
{
  fvwm_msg(ERR,"nocolor","can't %s color %s", note, name ? name : "<NONE>");
}
