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
#include "libs/FShape.h"
#include "libs/Colorset.h"
#include "libs/safemalloc.h"
#include "libs/PictureBase.h"
#include "libs/PictureGraphics.h"

/* globals */
colorset_struct *Colorset = NULL;
int nColorsets = 0;

/*****************************************************************************
 * AllocColorset() grows the size of the Colorset array to include set n
 * Colorset_struct *Colorset will be altered
 * returns the address of the member
 *****************************************************************************/
void AllocColorset(int n)
{
  /* do nothing if it already exists */
  if (n < nColorsets)
    return;

  /* increment n to get the required array size, get a new array */
  Colorset = (colorset_struct *)saferealloc((char *)Colorset,
					    ++n * sizeof(colorset_struct));

  /* zero out colorset 0
     it's always defined so will be filled in during module startup */
  if (n == 0)
    memset(&Colorset[nColorsets], 0, (n - nColorsets) * sizeof(colorset_struct));
  else
    /* copy colorset 0 into new members so that if undefined ones are
     * referenced at least they don't give black on black */
    for ( ; nColorsets < n; nColorsets++)
      memcpy(&Colorset[nColorsets], Colorset, sizeof(colorset_struct));

  nColorsets = n;
}

/*****************************************************************************
 * DumpColorset() returns a char * to the colorset contents in printable form
 *****************************************************************************/
static char csetbuf[256];
char *DumpColorset(int n, colorset_struct *cs)
{
  sprintf(csetbuf,
	  "Colorset "
	  "%x %lx %lx %lx %lx %lx %lx %lx %lx %lx "
	  "%x %x %x %x %x %x %x %x %x %x %x",
	  n, cs->fg, cs->bg, cs->hilite, cs->shadow, cs->fgsh, cs->tint,
	  cs->icon_tint, cs->pixmap, cs->shape_mask,
	  cs->fg_alpha, cs->width, cs->height, cs->pixmap_type,
	  cs->shape_width, cs->shape_height, cs->shape_type, cs->tint_percent,
	  cs->do_dither_icon, cs->icon_tint_percent, cs->icon_alpha);
  return csetbuf;
}

/*****************************************************************************
 * LoadColorset() takes a strings and stuffs it into the array
 *****************************************************************************/
int LoadColorset(char *line)
{
  colorset_struct *cs;
  unsigned int n, chars;
  Pixel fg, bg, hilite, shadow, fgsh, tint, icon_tint;
  Pixmap pixmap;
  Pixmap shape_mask;
  unsigned int fg_alpha, width, height, pixmap_type;
  unsigned int shape_width, shape_height, shape_type;
  unsigned int tint_percent, do_dither_icon, icon_tint_percent, icon_alpha;

  if (line == NULL)
    return -1;
  if (sscanf(line, "%x%n", &n, &chars) < 1)
    return -1;
  line += chars;
  if (sscanf(line,
	     "%lx %lx %lx %lx %lx %lx %lx %lx %lx " 
	     "%x %x %x %x %x %x %x %x %x %x %x",
	     &fg, &bg, &hilite, &shadow, &fgsh, &tint, &icon_tint, &pixmap,
	     &shape_mask,
	     &fg_alpha, &width, &height, &pixmap_type, &shape_width,
	     &shape_height, &shape_type, &tint_percent, &do_dither_icon,
	     &icon_tint_percent,
	     &icon_alpha) != 20)
    return -1;

  AllocColorset(n);
  cs = &Colorset[n];
  cs->fg = fg;
  cs->bg = bg;
  cs->hilite = hilite;
  cs->shadow = shadow;
  cs->fgsh = fgsh;
  cs->tint = tint;
  cs->icon_tint = icon_tint;
  cs->pixmap = pixmap;
  cs->shape_mask = shape_mask;
  cs->fg_alpha = fg_alpha;
  cs->width = width;
  cs->height = height;
  cs->pixmap_type = pixmap_type;
  cs->shape_width = shape_width;
  cs->shape_height = shape_height;
  cs->shape_type = shape_type;
  cs->tint_percent = tint_percent;
  cs->do_dither_icon = do_dither_icon;
  cs->icon_tint_percent = icon_tint_percent;
  cs->icon_alpha = icon_alpha;

  return n;
}

/* scrolls a pixmap by x_off/y_off pixels, wrapping around at the edges. */
Pixmap ScrollPixmap(
	Display *dpy, Pixmap p, GC gc, int x_off, int y_off, int width,
	int height, unsigned int depth)
{
	GC tgc;
	XGCValues xgcv;
	Pixmap p2;

	if (p == None || p == ParentRelative || (x_off == 0 && y_off == 0))
	{
		return p;
	}
	tgc = fvwmlib_XCreateGC(dpy, p, 0, &xgcv);
	if (tgc == None)
	{
		return p;
	}
	XCopyGC(dpy, gc, GCFunction | GCPlaneMask| GCSubwindowMode |
		GCClipXOrigin | GCClipYOrigin | GCClipMask, tgc);
	xgcv.tile = p;
	xgcv.ts_x_origin = x_off;
	xgcv.ts_y_origin = y_off;
	xgcv.fill_style = FillTiled;
	XChangeGC(
		dpy, tgc, GCTile | GCTileStipXOrigin | GCTileStipYOrigin |
		GCFillStyle, &xgcv);
	p2 = XCreatePixmap(dpy, p, width, height, depth);
	if (p2 == None)
	{
		return p;
	}
	XFillRectangle(dpy, p2, tgc, 0, 0, width, height);
	XFreeGC(dpy, tgc);

	return p2;
}

/* sets a window background from a colorset
 * if width or height are zero the window size is queried
 */
void SetWindowBackgroundWithOffset(
	Display *dpy, Window win, int x_off, int y_off, int width, int height,
	colorset_struct *colorset, unsigned int depth, GC gc, Bool clear_area)
{
	Pixmap pixmap = None;
	Pixmap mask = None;
	XID junk;

	if (0 == width || 0 == height)
	{
		if (!XGetGeometry(
			    dpy, win, &junk, (int *)&junk, (int *)&junk,
			    (unsigned int *)&width, (unsigned int *)&height,
			    (unsigned int *)&junk, (unsigned int *)&junk))
		{
			return;
		}
	}

	if (FHaveShapeExtension && colorset->shape_mask)
	{
		mask = CreateBackgroundPixmap(
			dpy, 0, width, height, colorset, 1, None, True);
		if (mask != None)
		{
			FShapeCombineMask(
				dpy, win, FShapeBounding, 0, 0, mask,
				FShapeSet);
			XFreePixmap(dpy, mask);
		}
	}
	if (!colorset->pixmap)
	{
		/* use the bg pixel */
		XSetWindowBackground(dpy, win, colorset->bg);
		if (clear_area)
		{
			XClearArea(dpy, win, 0, 0, width, height, True);
		}
	}
	else
	{

		pixmap = CreateBackgroundPixmap(
			dpy, win, width, height, colorset, depth, gc, False);
		if (x_off != 0 || y_off != 0)
		{
			Pixmap p2;

			p2 = ScrollPixmap(
				dpy, pixmap, gc, x_off, y_off, width, height,
				depth);
			if (p2 != None && p2 != ParentRelative && p2 != pixmap)
			{
				XFreePixmap(dpy, pixmap);
				pixmap = p2;
			}
		}
		if (pixmap)
		{
			XSetWindowBackgroundPixmap(dpy, win, pixmap);
			if (clear_area)
			{
				XClearArea(dpy, win, 0, 0, width, height, True);
			}
			if (pixmap != ParentRelative)
			{
				XFreePixmap(dpy, pixmap);
			}
		}
	}

	return;
}

void UpdateBackgroundTransparency(
	Display *dpy, Window win, int width, int height,
	colorset_struct *colorset, unsigned int depth, GC gc, Bool clear_area)
{
	if (!CSETS_IS_TRANSPARENT(colorset))
	{
		return;
	}
	else if (!CSETS_IS_TRANSPARENT_PR_PURE(colorset))
	{
		SetWindowBackgroundWithOffset(
			dpy, win, 0, 0, width, height, colorset, depth, gc,
			True);
	}
	else
	{
		XClearArea(dpy, win, 0,0,0,0, clear_area);
	}

	return;
}

void SetWindowBackground(
	Display *dpy, Window win, int width, int height,
	colorset_struct *colorset, unsigned int depth, GC gc, Bool clear_area)
{
	SetWindowBackgroundWithOffset(
		dpy, win, 0, 0, width, height, colorset, depth, gc, clear_area);

	return;
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
  GC fill_gc = None; /* not static as dpy may change (FvwmBacker) */
  int cs_width;
  int cs_height;
  Bool cs_keep_aspect;
  Bool cs_stretch_x;
  Bool cs_stretch_y;

  if (colorset->pixmap == ParentRelative && !is_shape_mask &&
      colorset->tint_percent > 0)
  {
	  FvwmRenderAttributes fra;

	  fra.mask = FRAM_DEST_IS_A_WINDOW | FRAM_HAVE_TINT;
	  fra.tint = colorset->tint;
	  fra.tint_percent = colorset->tint_percent;
	  XGrabServer(dpy);
	  pixmap = PGraphicsCreateTransprency(
		  dpy, win, &fra, gc, 0, 0, width, height, True);
	  XUngrabServer(dpy);
	  if (pixmap == None)
	  {
		  return ParentRelative;
	  }
	  return pixmap;
  }
  else if (colorset->pixmap == ParentRelative && !is_shape_mask)
  {
	  return ParentRelative;
  }
  else if (CSETS_IS_TRANSPARENT_ROOT(colorset) && colorset->pixmap
	   && !is_shape_mask)
  {
	  int sx,sy;
	  int h,w;
	  XID dummy;
	  
	  cs_pixmap = colorset->pixmap;
	  cs_width = colorset->width;
	  cs_height = colorset->height;
	  if (CSETS_IS_TRANSPARENT_ROOT_PURE(colorset))
	  {
		  /* check if it is still here */
		  XID dummy;
		  
		  if (!XGetGeometry(
			  dpy, colorset->pixmap, &dummy, (int *)&dummy,
			  (int *)&dummy,
			  (unsigned int *)&w, (unsigned int *)&h,
			  (unsigned int *)&dummy, (unsigned int *)&dummy))
		  {
			  return None;
		  }
		  if (w != cs_width || h != cs_height)
		  {
			  return None;
		  }
	  }
	  XTranslateCoordinates(
		dpy, win, DefaultRootWindow(dpy), 0, 0, &sx, &sy, &dummy);
	  pixmap = XCreatePixmap(dpy, win, width, height, Pdepth);
	  if (!pixmap)
	  {
		  return None;
	  }
	  /* make sx and sy positif */
	  while(sx < 0)
	  {
		  sx = sx + cs_width;
	  }
	  while(sy < 0)
	  {
		  sy = sy + cs_height;
	  }
	  /* make sx and sy in (0,0,cs_width,cs_height) */
	  while(sx >= cs_width)
	  {
		  sx = sx - cs_width;
	  }
	  while(sy >= cs_height)
	  {
		  sy = sy - cs_height;
	  }
	  xgcv.fill_style = FillTiled;
	  xgcv.tile = cs_pixmap;
	  xgcv.ts_x_origin = cs_width-sx;
	  xgcv.ts_y_origin = cs_height-sy;
	  fill_gc = fvwmlib_XCreateGC(
		  dpy, win, GCTile | GCTileStipXOrigin | GCTileStipYOrigin
		  | GCFillStyle, &xgcv);
	  XFillRectangle(dpy, pixmap, fill_gc, 0, 0, width, height);
	  if (CSETS_IS_TRANSPARENT_ROOT_PURE(colorset) &&
	      colorset->tint_percent > 0)
	  {
		  FvwmRenderAttributes fra;

		  fra.mask = FRAM_HAVE_TINT;
		  fra.tint = colorset->tint;
		  fra.tint_percent = colorset->tint_percent;
		  PGraphicsRenderPixmaps(
			  dpy, win, pixmap, None, None,
			  Pdepth, &fra, pixmap,
			  gc, None, None,
			  0, 0, width, height,
			  0, 0, width, height, False);
	  }
	  return pixmap;
  }
  if (!is_shape_mask)
  {
	  cs_pixmap = colorset->pixmap;
	  cs_width = colorset->width;
	  cs_height = colorset->height;
	  cs_keep_aspect = (colorset->pixmap_type == PIXMAP_STRETCH_ASPECT);
	  cs_stretch_x = (colorset->pixmap_type == PIXMAP_STRETCH_X)
		  || (colorset->pixmap_type == PIXMAP_STRETCH);
	  cs_stretch_y = (colorset->pixmap_type == PIXMAP_STRETCH_Y)
		  || (colorset->pixmap_type == PIXMAP_STRETCH);
  }
  else
  {
	  /* In spite of the name, win contains the pixmap */
	  cs_pixmap = colorset->shape_mask;
	  win = colorset->shape_mask;
	  if (shape_gc == None)
	  {
		  xgcv.foreground = 1;
		  xgcv.background = 0;
		  /* create a gc for 1 bit depth */
		  shape_gc = fvwmlib_XCreateGC(
			  dpy, win, GCForeground|GCBackground, &xgcv);
	  }
	  gc = shape_gc;
	  cs_width = colorset->shape_width;
	  cs_height = colorset->shape_height;
	  cs_keep_aspect = (colorset->shape_type == SHAPE_STRETCH_ASPECT);
	  cs_stretch_x = !(colorset->shape_type == SHAPE_TILED);
	  cs_stretch_y = !(colorset->shape_type == SHAPE_TILED);
  }

  if (cs_pixmap == None)
  {
	  xgcv.foreground = colorset->bg;
	  fill_gc = fvwmlib_XCreateGC(dpy, win, GCForeground, &xgcv);
	  /* create a solid pixmap - not very useful most of the time */
	  pixmap = XCreatePixmap(dpy, win, width, height, depth);
	  XFillRectangle(dpy, pixmap, fill_gc, 0, 0, width, height);
  }
  else if (cs_keep_aspect)
  {
	  Bool trim_side;
	  int big_width, big_height;
	  Pixmap big_pixmap;
	  int x, y;

	  /* do sides need triming or top/bottom? */
	  trim_side = (cs_width * height > cs_height * width);

	  /* make a pixmap big enough to cover the destination but with the
	   * aspect ratio of the cs_pixmap */
	  big_width = trim_side ? height * cs_width / cs_height : width;
	  big_height = trim_side ? height : width * cs_width / cs_width;
	  big_pixmap = CreateStretchPixmap(
		  dpy, cs_pixmap, cs_width, cs_height, depth, big_width,
		  big_height, gc);

	  /* work out where to trim */
	  x = trim_side ? (big_width - width) / 2 : 0;
	  y = trim_side ? 0 : (big_height - height) / 2;

	  pixmap = XCreatePixmap(dpy, cs_pixmap, width, height, depth);
	  if (pixmap && big_pixmap)
	  {
		  XCopyArea(
			  dpy, big_pixmap, pixmap, gc, x, y, width, height,
			  0, 0);
	  }
	  if (big_pixmap)
	  {
		  XFreePixmap(dpy, big_pixmap);
	  }
  }
  else if (!cs_stretch_x && !cs_stretch_y) 
  {
	  /* it's a tiled pixmap, create an unstretched one */
	  if (!is_shape_mask)
	  {
		  pixmap = XCreatePixmap(
			  dpy, cs_pixmap, cs_width, cs_height, depth);
		  if (pixmap)
		  {
			  XCopyArea(dpy, cs_pixmap, pixmap, gc, 0, 0, cs_width,
				    cs_height, 0, 0);
		  }
	  }
	  else
	  {
		  /* can't tile masks, create a tiled version of the mask */
		  pixmap = CreateTiledPixmap(
			  dpy, cs_pixmap, cs_width, cs_height, width, height,
			  1, gc);
	  }
  }
  else if (!cs_stretch_x)
  {
	  /* it's an HGradient */
	  pixmap = CreateStretchYPixmap(
		  dpy, cs_pixmap, cs_width, cs_height, depth, height, gc);
  }
  else if (!cs_stretch_y)
  {
	  /* it's a VGradient */
	  pixmap = CreateStretchXPixmap(
		  dpy, cs_pixmap, cs_width, cs_height, depth, width, gc);
  }
  else
  {
	  /* It's a full window pixmap */
	  pixmap = CreateStretchPixmap(
		  dpy, cs_pixmap, cs_width, cs_height, depth, width, height, gc);
  }

  return pixmap;
}


/* Draws a colorset background into the specified rectangle in the target
 * drawable. */
void SetRectangleBackground(
  Display *dpy, Window win, int x, int y, int width, int height,
  colorset_struct *colorset, unsigned int depth, GC gc)
{
  GC draw_gc;
  Pixmap pixmap2;
  Pixmap pixmap = None;
  static int last_depth = -1;
  static GC last_gc = None;
  XGCValues xgcv;
  Pixmap clipmask = None;
  GC clip_gc = None;
  Bool keep_aspect = (colorset->pixmap_type == PIXMAP_STRETCH_ASPECT);
  Bool stretch_x = (colorset->pixmap_type == PIXMAP_STRETCH_X)
		   || (colorset->pixmap_type == PIXMAP_STRETCH);
  Bool stretch_y = (colorset->pixmap_type == PIXMAP_STRETCH_Y)
		   || (colorset->pixmap_type == PIXMAP_STRETCH);

  if (colorset->pixmap == ParentRelative && colorset->tint_percent > 0)
  {
	  XClearArea(dpy, win, x, y, width, height, False);
	  PGraphicsTintRectangle(
		  dpy, win, colorset->tint, colorset->tint_percent,
		  win, True, gc, None, None,
		  x, y, width, height);
	  return;
  }
  if (colorset->pixmap == ParentRelative)
  {
    XClearArea(dpy, win, x, y, width, height, False);
    /* don't do anything */
    return;
  }
  /* minimize gc creation by remembering the last requested depth */
  if (last_gc != None && depth != last_depth)
  {
    XFreeGC(dpy, last_gc);
    last_gc = None;
  }
  if (last_gc == None)
  {
    last_gc = fvwmlib_XCreateGC(dpy, win, 0, &xgcv);
  }
  draw_gc = last_gc;
  last_depth = depth;

  if (FHaveShapeExtension && colorset->shape_mask != None)
  {
    clipmask = CreateBackgroundPixmap(
      dpy, 0, width, height, colorset, 1, None, True);
    if (clipmask)
    {
      /* create a GC for clipping */
      xgcv.clip_x_origin = x;
      xgcv.clip_y_origin = y;
      xgcv.clip_mask = clipmask;
      clip_gc = fvwmlib_XCreateGC(
	dpy, win, GCClipXOrigin|GCClipYOrigin|GCClipMask, &xgcv);
      draw_gc = clip_gc;
    }
  }

  if (!colorset->pixmap)
  {
    /* use the bg pixel */
    XSetForeground(dpy, draw_gc, colorset->bg);
    XFillRectangle(dpy, win, draw_gc, x, y, width, height);
  }
  else
  {
    pixmap = CreateBackgroundPixmap(
      dpy, win, width, height, colorset, depth, gc, False);
    if (keep_aspect)
    {
      /* nothing to do */
    }
    if (stretch_x || stretch_y)
    {
      if (!stretch_x && colorset->width != width)
      {
	pixmap2 = CreateStretchXPixmap(
	  dpy, pixmap, colorset->width, height, depth, width, gc);
	XFreePixmap(dpy, pixmap);
	pixmap = pixmap2;
      }
      if (!stretch_y && colorset->height != height)
      {
	pixmap2 = CreateStretchYPixmap(
	  dpy, pixmap, width, colorset->height, depth, width, gc);
	XFreePixmap(dpy, pixmap);
	pixmap = pixmap2;
      }
    }
    else
    {
      pixmap2 = CreateTiledPixmap(
	dpy, pixmap, colorset->width, colorset->height, width, height, depth,
	gc);
      XFreePixmap(dpy, pixmap);
      pixmap = pixmap2;
    }

    if (pixmap)
    {
      /* Copy the pixmap into the rectangle. */
      XCopyArea(dpy, pixmap, win, draw_gc, 0, 0, width, height, x, y);
      XFreePixmap(dpy, pixmap);
    }
  }

  if (FHaveShapeExtension)
  {
    if (clipmask != None)
      XFreePixmap(dpy, clipmask);
    if (clip_gc != None)
      XFreeGC(dpy, clip_gc);
  }
}
