/* Fvwm colorset technology is Copyright (C) 1999 Joey Shutup
 * http://www.streetmap.co.uk/streetmap.dll?Postcode2Map?BS24+9TZ
 * You may use this code for any purpose, as long as the original copyright
 * and this notice remains in the source code and all documentation
*/
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.GPL.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 *
 */


#include "config.h"                     /* must be first */

#include <stdio.h>
#include <X11/Intrinsic.h>

#include "libs/fvwmlib.h"
#include "libs/Colorset.h"

/* globals */
colorset_struct *Colorset = NULL;
unsigned int nColorsets = 0;

/*****************************************************************************
 * AllocColorset() grows the size of the Colorset array to include set n
 * Colorset_struct *Colorset will be altered
 * returns the address of the member
 *****************************************************************************/
colorset_struct *AllocColorset(unsigned int n) {
  colorset_struct *newColorset;

  /* do nothing if it already exists */
  if (n < nColorsets)
    return &Colorset[n];

  /* alloc space for the whole new array */
  newColorset = (colorset_struct *)calloc(n + 1, sizeof(colorset_struct));
  if (!newColorset) {
    fprintf(stderr, "calloc() of Colorset %d failed. Exiting\n", n);
    exit(1);
  }
  
  /* copy and discard the old array */
  if (Colorset) {
    memcpy(newColorset, Colorset, sizeof(colorset_struct) * nColorsets);
    free(Colorset);
  }
  
  Colorset = newColorset;
  nColorsets = n + 1;
  
  return &Colorset[n];
}

/*****************************************************************************
 * DumpColorset() returns a char * to the colorset contents in printable form
 *****************************************************************************/
static char csetbuf[64];
char *DumpColorset(unsigned int n) {
  colorset_struct *cs = &Colorset[n];

  sprintf(csetbuf, "Colorset %x %lx %lx %lx %lx %lx %x %x %x %x %x", n, cs->fg,
	  cs->bg, cs->hilite, cs->shadow, cs->pixmap, cs->width, cs->height,
	  cs->stretch_x, cs->stretch_y, cs->keep_aspect);
  return csetbuf;
}

/*****************************************************************************
 * LoadColorset() takes a strings and stuffs it into the array
 *****************************************************************************/
int LoadColorset(char *line) {
  colorset_struct *cs;
  unsigned int n, chars;
  Pixel fg, bg, hilite, shadow;
  Pixmap pixmap;
  unsigned int width, height, stretch_x, stretch_y, keep_aspect;

  if (sscanf(line, "%d%n", &n, &chars) != 1)
    return -1;
  line += chars;
  if (sscanf(line, "%lx %lx %lx %lx %lx %x %x %x %x %x", &fg, &bg, &hilite,
	     &shadow, &pixmap, &width, &height, &stretch_x, &stretch_y,
	     &keep_aspect) != 10)
    return -1;
  AllocColorset(n);
  cs = &Colorset[n];
  cs->fg = fg;
  cs->bg = bg;
  cs->hilite = hilite;
  cs->shadow = shadow;
  cs->pixmap = pixmap;
  cs->width = width;
  cs->height = height;
  cs->stretch_x = stretch_x;
  cs->stretch_y = stretch_y;
  cs->keep_aspect = keep_aspect;
  return n;
}

/* sets a window background from a colorset
 */
void SetWindowBackground(Display *dpy, Window win, int width, int height,
			 colorset_struct *colorset, unsigned int depth, GC gc)
{
  Pixmap pixmap = None;

  if (!colorset->pixmap) {
    /* use the bg pixel */
    XSetWindowBackground(dpy, win, colorset->bg);
  /* fixme: test for keep_aspect here */
  } else if (!colorset->stretch_x && !colorset->stretch_y) {
    /* it's a tiled pixmap */
    XSetWindowBackgroundPixmap(dpy, win, colorset->pixmap);
  } else if (!colorset->stretch_x) {
    /* it's an VGradient */
    pixmap = CreateStretchYPixmap(dpy, colorset->pixmap, colorset->width,
				  colorset->height, depth, height, gc);
  } else if (!colorset->stretch_y) {
    /* it's a VGradient */
    pixmap = CreateStretchXPixmap(dpy, colorset->pixmap, colorset->width,
				 colorset->height, depth, width, gc);
  } else {
    /* It's a full window pixmap */
    pixmap = CreateStretchPixmap(dpy, colorset->pixmap, colorset->width,
				 colorset->height, depth, width, height, gc);
  }

  if (pixmap)
    XSetWindowBackgroundPixmap(dpy, win, pixmap);
  XClearArea(dpy, win, 0, 0, width, height, True);
  XFlush(dpy);
  if (pixmap)
    XFreePixmap(dpy, pixmap);
}
