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

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

#include "libs/fvwmlib.h"
#include "libs/Colorset.h"
#include "libs/Picture.h"
#include "libs/safemalloc.h"

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
  newColorset = (colorset_struct *)safecalloc(n + 1, sizeof(colorset_struct));

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
static char csetbuf[160];
char *DumpColorset(unsigned int n)
{
  colorset_struct *cs = &Colorset[n];

  sprintf(csetbuf,
	  "Colorset %x %lx %lx %lx %lx %lx %lx %lx %x %x %x %x %x %x %x",
	  n, cs->fg,
	  cs->bg, cs->hilite, cs->shadow, cs->pixmap, cs->mask, cs->shape_mask,
	  cs->width, cs->height, cs->stretch_x, cs->stretch_y,
	  cs->keep_aspect, cs->shape_tile, cs->shape_keep_aspect);
  return csetbuf;
}

/*****************************************************************************
 * LoadColorset() takes a strings and stuffs it into the array
 *****************************************************************************/
static int LoadColorsetConditional(char *line, Bool free)
{
  colorset_struct *cs;
  unsigned int n, chars;
  Pixel fg, bg, hilite, shadow;
  Pixmap pixmap;
  Pixmap mask;
  Pixmap shape_mask;
  unsigned int width, height, stretch_x, stretch_y, keep_aspect;
  unsigned int shape_tile, shape_keep_aspect;

  if (line == NULL)
    return -1;
  if (sscanf(line, "%x%n", &n, &chars) != 1)
    return -1;
  line += chars;
  if (sscanf(line, "%lx %lx %lx %lx %lx %lx %lx %x %x %x %x %x %x %x",
	     &fg, &bg, &hilite, &shadow, &pixmap, &mask, &shape_mask, &width,
	     &height, &stretch_x, &stretch_y, &keep_aspect, &shape_tile,
	     &shape_keep_aspect) != 14)
    return -1;
  AllocColorset(n);
  cs = &Colorset[n];
  if (free) {
    if (cs->fg != fg) {
      XFreeColors(Pdpy, Pcmap, &cs->fg, 1, 0);
    }
    if (cs->bg != bg) {
      XFreeColors(Pdpy, Pcmap, &cs->bg, 1, 0);
      XFreeColors(Pdpy, Pcmap, &cs->hilite, 1, 0);
      XFreeColors(Pdpy, Pcmap, &cs->shadow, 1, 0);
    }
    if (cs->pixmap && (cs->pixmap != pixmap)) {
      XFreePixmap(Pdpy, cs->pixmap);
    }
    if (cs->mask && (cs->mask != mask)) {
      XFreePixmap(Pdpy, cs->mask);
    }
    if (cs->shape_mask && (cs->shape_mask != mask)) {
      XFreePixmap(Pdpy, cs->shape_mask);
    }
  }
  cs->fg = fg;
  cs->bg = bg;
  cs->hilite = hilite;
  cs->shadow = shadow;
  cs->pixmap = pixmap;
  cs->mask = mask;
  cs->shape_mask = shape_mask;
  cs->width = width;
  cs->height = height;
  cs->stretch_x = stretch_x;
  cs->stretch_y = stretch_y;
  cs->keep_aspect = keep_aspect;
  cs->shape_tile = shape_tile;
  cs->shape_keep_aspect = shape_keep_aspect;
  return n;
}
inline int LoadColorset(char *line)
{
  return LoadColorsetConditional(line, False);
}
inline int LoadColorsetAndFree(char *line)
{
  return LoadColorsetConditional(line, True);
}

/* sets a window background from a colorset
 * if width or height are zero the window size is queried
 */
void SetWindowBackground(Display *dpy, Window win, int width, int height,
			 colorset_struct *colorset, unsigned int depth, GC gc)
{
  Pixmap pixmap = None;
  Pixmap mask = None;
  XID junk;

  if (0 == width || 0 == height)
    XGetGeometry(dpy, win, &junk, (int *)&junk, (int *)&junk, &width, &height,
		 (unsigned int *)&junk, (unsigned int *)&junk);

#ifdef SHAPE
  if (colorset->shape_mask)
  {
    mask = CreateBackgroundPixmap(
      dpy, 0, width, height, colorset, 1, None, True);
    if (mask)
    {
      XShapeCombineMask(dpy, win, ShapeBounding, 0, 0, mask, ShapeSet);
      XFreePixmap(dpy, mask);
    }
  }
#endif
  if (!colorset->pixmap) {
    /* use the bg pixel */
    XSetWindowBackground(dpy, win, colorset->bg);
    XClearArea(dpy, win, 0, 0, width, height, True);
  } else {
    pixmap = CreateBackgroundPixmap(dpy, win, width, height, colorset, depth,
				    gc, False);
    if (pixmap) {
      XSetWindowBackgroundPixmap(dpy, win, pixmap);
      XClearArea(dpy, win, 0, 0, width, height, True);
      XFreePixmap(dpy, pixmap);
    }
  }
}

/* create a pixmap suitable for plonking on the background of a window */
Pixmap CreateBackgroundPixmap(Display *dpy, Window win, int width, int height,
			      colorset_struct *colorset, unsigned int depth,
			      GC gc, Bool is_shape_mask)
{
  Pixmap pixmap = None;
  Pixmap cs_pixmap = None;
  XGCValues xgcv;
  static GC shape_gc = None;
  int cs_width;
  int cs_height;
  unsigned int cs_keep_aspect;
  unsigned int cs_stretch_x;
  unsigned int cs_stretch_y;

  if (!is_shape_mask)
  {
    cs_pixmap = colorset->pixmap;
    cs_width = colorset->width;
    cs_height = colorset->height;
    cs_keep_aspect = colorset->keep_aspect;
    cs_stretch_x = colorset->stretch_x;
    cs_stretch_y = colorset->stretch_y;
  }
  else
  {
    Window junkw;
    int junk;

    /* In spite of the name, win contains the pixmap */
    cs_pixmap = colorset->shape_mask;
    win = colorset->shape_mask;
    if (shape_gc == None)
    {
      xgcv.foreground = 1;
      xgcv.background = 0;
      /* create a gc for 1 bit depth */
      shape_gc = XCreateGC(dpy, win, GCForeground|GCBackground, &xgcv);
    }
    gc = shape_gc;
    /* fetch the width and height of the shape window. Shapes are probably used
     * only rearely, so it does not make sense to communicate the dimensions
     * in the colorset structure. */
    if (!XGetGeometry(dpy, colorset->shape_mask, &junkw, &junk, &junk,
		      &cs_width, &cs_height, &junk, &junk))
    {
      return None;
    }
    cs_keep_aspect = colorset->shape_keep_aspect;
    cs_stretch_x = !(colorset->shape_tile);
    cs_stretch_y = !(colorset->shape_tile);
  }

  if (cs_keep_aspect) {
    Bool trim_side;
    int big_width, big_height;
    Pixmap big_pixmap;
    int x, y;

    /* do sides need triming or top/bottom? */
    trim_side = (cs_width * height > cs_height * width);

    /* make a pixmap big enough to cover the destination but with the aspect
     * ratio of the cs_pixmap */
    big_width = trim_side ? height * cs_width / cs_height : width;
    big_height = trim_side ? height : width * cs_width / cs_width;
    big_pixmap = CreateStretchPixmap(
      dpy, cs_pixmap, cs_width, cs_height, depth, big_width, big_height, gc);

    /* work out where to trim */
    x = trim_side ? (big_width - width) / 2 : 0;
    y = trim_side ? 0 : (big_height - height) / 2;

    pixmap = XCreatePixmap(dpy, cs_pixmap, width, height, depth);
    if (pixmap && big_pixmap)
      XCopyArea(dpy, big_pixmap, pixmap, gc, x, y, width, height, 0, 0);
    if (big_pixmap)
      XFreePixmap(dpy, big_pixmap);
  } else if (!cs_stretch_x && !cs_stretch_y) {
    /* it's a tiled pixmap, create an unstretched one */
    if (!is_shape_mask)
    {
      pixmap = XCreatePixmap(dpy, cs_pixmap, cs_width, cs_height, depth);
      if (pixmap)
      {
	XCopyArea(dpy, cs_pixmap, pixmap, gc, 0, 0, cs_width,
		  cs_height, 0, 0);
      }
    }
    else
    {
      /* can't tile masks, create a tiled version of the mask */
      pixmap = CreateTiledMaskPixmap(
	dpy, cs_pixmap, cs_width, cs_height, width, height, gc);
    }
  } else if (!cs_stretch_x) {
    /* it's an HGradient */
    pixmap = CreateStretchYPixmap(dpy, cs_pixmap, cs_width,
				  cs_height, depth, height, gc);
  } else if (!cs_stretch_y) {
    /* it's a VGradient */
    pixmap = CreateStretchXPixmap(dpy, cs_pixmap, cs_width,
				  cs_height, depth, width, gc);
  } else {
    /* It's a full window pixmap */
    pixmap = CreateStretchPixmap(dpy, cs_pixmap, cs_width,
				 cs_height, depth, width, height, gc);
  }

  return pixmap;
}
