/* -*-c-*- */
/* Fvwm colorset technology is Copyright (C) 1999 Joey Shutup
 * http://www.streetmap.co.uk/streetmap.dll?Postcode2Map?BS24+9TZ
 * You may use this code for any purpose, as long as the original copyright
 * and this notice remains in the source code and all documentation
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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include <stdio.h>
#include <X11/Intrinsic.h>

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/Colorset.h"
#include "libs/PictureBase.h"
#include "libs/Graphics.h"
#include "libs/Grab.h"
#include "libs/PictureGraphics.h"
#include "libs/XError.h"

/* globals */
colorset_t *Colorset = NULL;
int nColorsets = 0;

/* stretch the src rectangle to the dest ractangle keeping its aspect so that
 * it fills the destination completely. */
static int get_aspect_dimensions(
	int *ret_w, int *ret_h, int dest_w, int dest_h, int src_w, int src_h)
{
	double ax;
	double ay;

	ax = (double)dest_w / (double)src_w;
	ay = (double)dest_h / (double)src_h;
	if (ax >= ay)
	{
		/* fit in x direction */
		*ret_w = dest_w;
		*ret_h = (src_h * dest_w) / src_w;
		return 0;
	}
	else
	{
		/* fit in y direction */
		*ret_w = (src_w * dest_h) / src_h;
		*ret_h = dest_h;
		return 1;
	}
}

/*
 * AllocColorset() grows the size of the Colorset array to include set n
 * colorset_t *Colorset will be altered
 * returns the address of the member
 */
void AllocColorset(int n)
{
	/* do nothing if it already exists */
	if (n < nColorsets)
	{
		return;
	}

	/* increment n to get the required array size, get a new array */
	Colorset = xrealloc((void *)Colorset, ++n, sizeof(colorset_t));

	/* zero out colorset 0
	   it's always defined so will be filled in during module startup */
	if (n == 0)
	{
		memset(
			&Colorset[nColorsets], 0,
			(n - nColorsets) * sizeof(colorset_t));
	}
	else
	{
		/* copy colorset 0 into new members so that if undefined ones
		 * are referenced at least they don't give black on black */
		for ( ; nColorsets < n; nColorsets++)
		{
			memcpy(
				&Colorset[nColorsets], Colorset,
				sizeof(colorset_t));
		}
	}
	nColorsets = n;

	return;
}

/*
 * DumpColorset() returns a char * to the colorset contents in printable form
 */
static char csetbuf[256];
char *DumpColorset(int n, colorset_t *cs)
{
	sprintf(csetbuf,
		"Colorset "
		"%x %lx %lx %lx %lx %lx %lx %lx %lx %lx "
		"%x %x %x %x %x %x %x %x %x %x %x",
		n, cs->fg, cs->bg, cs->hilite, cs->shadow, cs->fgsh, cs->tint,
		cs->icon_tint, cs->pixmap, cs->shape_mask,
		cs->fg_alpha_percent, cs->width, cs->height, cs->pixmap_type,
		cs->shape_width, cs->shape_height, cs->shape_type,
		cs->tint_percent, cs->do_dither_icon, cs->icon_tint_percent,
		cs->icon_alpha_percent);
	return csetbuf;
}

/*
 * LoadColorset() takes a strings and stuffs it into the array
 */
int LoadColorset(char *line)
{
	colorset_t *cs;
	unsigned int n, chars;
	Pixel fg, bg, hilite, shadow, fgsh, tint, icon_tint;
	Pixmap pixmap;
	Pixmap shape_mask;
	unsigned int fg_alpha_percent, width, height, pixmap_type;
	unsigned int shape_width, shape_height, shape_type;
	unsigned int tint_percent, do_dither_icon, icon_tint_percent;
	unsigned int icon_alpha_percent;

	if (line == NULL)
	{
		return -1;
	}
	if (sscanf(line, "%x%n", &n, &chars) < 1)
	{
		return -1;
	}
	line += chars;

	/* migo: if you modify this sscanf or other colorset definitions,
	 * please update perllib/FVWM/Tracker/Colorsets.pm too */
	if (sscanf(line,
		   "%lx %lx %lx %lx %lx %lx %lx %lx %lx "
		   "%x %x %x %x %x %x %x %x %x %x %x",
		   &fg, &bg, &hilite, &shadow, &fgsh, &tint, &icon_tint,
		   &pixmap, &shape_mask, &fg_alpha_percent, &width, &height,
		   &pixmap_type, &shape_width, &shape_height, &shape_type,
		   &tint_percent, &do_dither_icon, &icon_tint_percent,
		   &icon_alpha_percent) != 20)
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
	cs->fg_alpha_percent = fg_alpha_percent;
	cs->width = width;
	cs->height = height;
	cs->pixmap_type = pixmap_type;
	cs->shape_width = shape_width;
	cs->shape_height = shape_height;
	cs->shape_type = shape_type;
	cs->tint_percent = tint_percent;
	cs->do_dither_icon = do_dither_icon;
	cs->icon_tint_percent = icon_tint_percent;
	cs->icon_alpha_percent = icon_alpha_percent;

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
	Display *dpy, Window win, int x_off, int y_off, unsigned int width, unsigned int height,
	colorset_t *colorset, unsigned int depth, GC gc, Bool clear_area)
{
	Pixmap pixmap = None;
	Pixmap mask = None;
	union {
		XID junk;
		unsigned int ui_junk;
		int i_junk;
	} XID_int;
	if (0 == width || 0 == height)
	{
		if (!XGetGeometry(
			    dpy, win, &XID_int.junk,
			    &XID_int.i_junk, &XID_int.i_junk,
			    (unsigned int *)&width,
			    (unsigned int *)&height,
			    &XID_int.ui_junk,
			    &XID_int.ui_junk))
		{
			return;
		}
	}

	if (FHaveShapeExtension && colorset->shape_mask)
	{
		mask = CreateBackgroundPixmap(
			dpy, None, width, height, colorset, 1, None, True);
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

		pixmap = CreateOffsetBackgroundPixmap(
			dpy, win, x_off, y_off, width, height, colorset,
			depth, gc, False);
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

Bool UpdateBackgroundTransparency(
	Display *dpy, Window win, int width, int height,
	colorset_t *colorset, unsigned int depth, GC gc, Bool clear_area)
{
	if (!CSETS_IS_TRANSPARENT(colorset))
	{
		return False;
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

	return True;
}

void SetWindowBackground(
	Display *dpy, Window win, int width, int height,
	colorset_t *colorset, unsigned int depth, GC gc, Bool clear_area)
{
	SetWindowBackgroundWithOffset(
		dpy, win, 0, 0, width, height, colorset, depth, gc, clear_area);

	return;
}

void GetWindowBackgroundPixmapSize(
	colorset_t *cs_t, int width, int height, int *w, int *h)
{
	if (cs_t->pixmap == None)
	{
		*w = *h = 1;
	}
	else
	{
		*w = cs_t->width;
		*h = cs_t->height;
		switch (cs_t->pixmap_type)
		{
		case PIXMAP_STRETCH_ASPECT:
			get_aspect_dimensions(
				w, h, width, height, cs_t->width, cs_t->height);
			break;
		case PIXMAP_STRETCH_X:
			*w = width;
			break;
		case PIXMAP_STRETCH_Y:
			*h = height;
			break;
		default:
			break;
		}
	}
}

static int is_bad_gc = 0;
static int BadGCErrorHandler(Display *dpy, XErrorEvent *error)
{
	if (error->error_code == BadGC)
	{
		is_bad_gc = 1;

		return 0;
	}
	else
	{
		int rc;

		/* delegate error to original handler */
		rc = ferror_call_next_error_handler(dpy, error);

		return rc;
	}
}

/* create a pixmap suitable for plonking on the background of a part of a
 * window */
Pixmap CreateOffsetBackgroundPixmap(
	Display *dpy, Window win, int x, int y, int width, int height,
	colorset_t *colorset, unsigned int depth, GC gc, Bool is_shape_mask)
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
		MyXGrabServer(dpy);
		pixmap = PGraphicsCreateTransparency(
			dpy, win, &fra, gc, x, y, width, height, True);
		MyXUngrabServer(dpy);
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
			union {
				XID junk;
				unsigned int ui_junk;
				int i_junk;
			} XID_int;
			/* a priori we should grab the server, but this
			 * cause PositiveWrite error when you move a
			 * window with a transparent title bar */
			if (!XGetGeometry(
				    dpy, colorset->pixmap, &XID_int.junk,
				    &XID_int.i_junk, &XID_int.i_junk,
				    (unsigned int *)&w, (unsigned int *)&h,
				    &XID_int.ui_junk,
				    &XID_int.ui_junk) ||
			    w != cs_width || h != cs_height)
			{
				return None;
			}
		}
		XTranslateCoordinates(
			dpy, win, DefaultRootWindow(dpy), x, y, &sx, &sy,
			&dummy);
		pixmap = XCreatePixmap(dpy, win, width, height, Pdepth);
		if (!pixmap)
		{
			return None;
		}
		/* make sx and sy positif */
		while (sx < 0)
		{
			sx = sx + cs_width;
		}
		while (sy < 0)
		{
			sy = sy + cs_height;
		}
		/* make sx and sy in (0,0,cs_width,cs_height) */
		while (sx >= cs_width)
		{
			sx = sx - cs_width;
		}
		while (sy >= cs_height)
		{
			sy = sy - cs_height;
		}
		xgcv.fill_style = FillTiled;
		xgcv.tile = cs_pixmap;
		xgcv.ts_x_origin = cs_width-sx;
		xgcv.ts_y_origin = cs_height-sy;
		fill_gc = fvwmlib_XCreateGC(
			dpy, win, GCTile | GCTileStipXOrigin |
			GCTileStipYOrigin | GCFillStyle, &xgcv);
		if (fill_gc == None)
		{
			XFreePixmap(dpy, pixmap);
			return None;
		}
		XSync(dpy, False);
		is_bad_gc = 0;
		ferror_set_temp_error_handler(BadGCErrorHandler);
		XFillRectangle(dpy, pixmap, fill_gc, 0, 0, width, height);
		if (
			is_bad_gc == 0 &&
			CSETS_IS_TRANSPARENT_ROOT_PURE(colorset) &&
			colorset->tint_percent > 0)
		{
			FvwmRenderAttributes fra;

			fra.mask = FRAM_HAVE_TINT;
			fra.tint = colorset->tint;
			fra.tint_percent = colorset->tint_percent;
			PGraphicsRenderPixmaps(
				dpy, win, pixmap, None, None,
				Pdepth, &fra, pixmap,
				fill_gc, None, None,
				0, 0, width, height,
				0, 0, width, height, False);
		}
		XSync(dpy, False);
		ferror_reset_temp_error_handler();
		if (is_bad_gc == 1)
		{
			is_bad_gc = 0;
			XFreePixmap(dpy, pixmap);
			pixmap = None;
		}
		XFreeGC(dpy,fill_gc);
		return pixmap;
	}
	if (!is_shape_mask)
	{
		cs_pixmap = colorset->pixmap;
		cs_width = colorset->width;
		cs_height = colorset->height;
		cs_keep_aspect =
			(colorset->pixmap_type == PIXMAP_STRETCH_ASPECT);
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
		pixmap = XCreatePixmap(dpy, win, 1, 1, depth);
		XFillRectangle(dpy, pixmap, fill_gc, 0, 0, 1, 1);
		XFreeGC(dpy,fill_gc);
	}
	else if (cs_keep_aspect)
	{
		Bool trim_side;
		int big_width, big_height;
		Pixmap big_pixmap;
		int x, y;

		/* make a pixmap big enough to cover the destination but with
		 * the aspect ratio of the cs_pixmap */
		trim_side = get_aspect_dimensions(
			&big_width, &big_height, width, height, cs_width,
			cs_height);
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
				dpy, big_pixmap, pixmap, gc, x, y, width,
				height, 0, 0);
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
				XCopyArea(
					dpy, cs_pixmap, pixmap, gc, 0, 0,
					cs_width, cs_height, 0, 0);
			}
		}
		else
		{
			/* can't tile masks, create a tiled version of the
			 * mask */
			pixmap = CreateTiledPixmap(
				dpy, cs_pixmap, cs_width, cs_height, width,
				height, 1, gc);
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
			dpy, cs_pixmap, cs_width, cs_height, depth, width,
			height, gc);
	}

	if (x != 0 || y != 0)
	{
		Pixmap p2;

		p2 = ScrollPixmap(
			dpy, pixmap, gc, x, y, width, height,
			depth);
		if (p2 != None && p2 != ParentRelative && p2 != pixmap)
		{
			XFreePixmap(dpy, pixmap);
			pixmap = p2;
		}
	}

	return pixmap;
}

/* create a pixmap suitable for plonking on the background of a window */
Pixmap CreateBackgroundPixmap(Display *dpy, Window win, int width, int height,
			      colorset_t *colorset, unsigned int depth,
			      GC gc, Bool is_shape_mask)
{
	return CreateOffsetBackgroundPixmap(
		dpy, win, 0, 0, width, height, colorset, depth, gc,
		is_shape_mask);
}

/* Draws a colorset background into the specified rectangle in the target
 * drawable. */
void SetRectangleBackground(
	Display *dpy, Window win, int x, int y, int width, int height,
	colorset_t *colorset, unsigned int depth, GC gc)
{
	SetClippedRectangleBackground(
		dpy, win, x, y, width, height, NULL, colorset, depth, gc);

	return;
}

/* Draws a colorset background into the specified rectangle in the target
 * drawable. */
void SetClippedRectangleBackground(
	Display *dpy, Window win, int x, int y, int width, int height,
	XRectangle *clip, colorset_t *colorset,
	unsigned int depth, GC gc)
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
	int dest_x, dest_y, dest_w, dest_h;

	if (clip)
	{
		dest_x = clip->x;
		dest_y = clip->y;
		dest_w = clip->width;
		dest_h = clip->height;
	}
	else
	{
		dest_x = x;
		dest_y = y;
		dest_w = width;
		dest_h = height;
	}
	if (CSETS_IS_TRANSPARENT_PR_TINT(colorset))
	{
		XClearArea(dpy, win, dest_x, dest_y, dest_w, dest_h, False);
		PGraphicsTintRectangle(
			dpy, win, colorset->tint, colorset->tint_percent,
			win, True, gc, None, None,
			dest_x, dest_y, dest_w, dest_h);
		return;
	}
	if (CSETS_IS_TRANSPARENT_PR_PURE(colorset))
	{
		XClearArea(dpy, win, dest_x, dest_y, dest_w, dest_h, False);
		/* don't do anything */
		return;
	}
	if (CSETS_IS_TRANSPARENT_ROOT(colorset))
	{
		/* FIXME: optimize this ! */
		x = y = 0;
		width = width + dest_x;
		height = height + dest_y;
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
				dpy, win, GCClipXOrigin | GCClipYOrigin |
				GCClipMask, &xgcv);
			draw_gc = clip_gc;
		}
	}

	if (!colorset->pixmap)
	{
		/* use the bg pixel */
		XSetForeground(dpy, draw_gc, colorset->bg);
		XFillRectangle(
			dpy, win, draw_gc, dest_x, dest_y, dest_w, dest_h);
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
					dpy, pixmap, colorset->width, height,
					depth, width, gc);
				XFreePixmap(dpy, pixmap);
				pixmap = pixmap2;
			}
			if (!stretch_y && colorset->height != height)
			{
				pixmap2 = CreateStretchYPixmap(
					dpy, pixmap, width, colorset->height,
					depth, height, gc);
				XFreePixmap(dpy, pixmap);
				pixmap = pixmap2;
			}
		}
		else
		{
			pixmap2 = CreateTiledPixmap(
				dpy, pixmap, colorset->width, colorset->height,
				width, height, depth, gc);
			XFreePixmap(dpy, pixmap);
			pixmap = pixmap2;
		}

		if (pixmap)
		{
			/* Copy the pixmap into the rectangle. */
			XCopyArea(
				dpy, pixmap, win, draw_gc,
				dest_x - x, dest_y - y, dest_w, dest_h,
				dest_x, dest_y);
			XFreePixmap(dpy, pixmap);
		}
	}

	if (FHaveShapeExtension)
	{
		if (clipmask != None)
		{
			XFreePixmap(dpy, clipmask);
		}
		if (clip_gc != None)
		{
			XFreeGC(dpy, clip_gc);
		}
	}

	return;
}
