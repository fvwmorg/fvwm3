/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis
 *
 * This program is free software; you can redistribute it and/or modify
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

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>

#include <X11/Xlib.h>

#include <fvwmlib.h>
#include "PictureBase.h"
#include "PictureImageLoader.h"
#include "FRenderInit.h"
#include "FRenderInterface.h"
#include "PictureGraphics.h"
#include "PictureUtils.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

static
void PCopyArea(Display *dpy, Pixmap pixmap, Pixmap mask, int depth,
	       Drawable d, GC gc,
	       int src_x, int src_y, int src_w, int src_h,
	       int dest_x, int dest_y)
{
	XGCValues gcv;
	unsigned long gcm;
	GC my_gc = None;

	if (gc == None)
	{
		my_gc = fvwmlib_XCreateGC(dpy, d, 0, NULL);
	}
	gcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
	gcv.clip_x_origin = dest_x;
	gcv.clip_y_origin = dest_y;
	if (depth == Pdepth)
	{
		gcv.clip_mask = mask;
		if (my_gc != None)
		{
			XChangeGC(dpy,my_gc,gcm,&gcv);
		}
		else
		{
			XChangeGC(dpy,gc,gcm,&gcv);
		}
		XCopyArea(dpy, pixmap, d,
			  (my_gc != None)? my_gc:gc,
			  src_x, src_y, src_w,src_h,
			  dest_x, dest_y);
	}
	else
	{
		/* monochrome bitmap */
		gcv.clip_mask = None;
		if (my_gc != None)
		{
			gcv.foreground = WhitePixel(dpy, DefaultScreen(dpy));
			gcv.background = BlackPixel(dpy, DefaultScreen(dpy));
			gcm |= GCBackground|GCForeground;
			XChangeGC(dpy,my_gc,gcm,&gcv);
		}
		else
		{
			XChangeGC(dpy,gc,gcm,&gcv);
		}
		XCopyPlane(dpy, pixmap, d,
			   (my_gc != None)? my_gc:gc,
			   src_x, src_y, src_w, src_h,
			   dest_x, dest_y, 1);
	}
	if (my_gc != None)
	{
		XFreeGC(dpy, my_gc);
	}
	else
	{
		gcm = GCClipMask;
		gcv.clip_mask = None;
		XChangeGC(dpy, gc, gcm, &gcv);
	}
}

static
void PTileRectangle(Display *dpy, Window win, Pixmap pixmap, Pixmap mask,
		    int depth,
		    int src_x, int src_y,
		    Drawable d, GC gc, GC mono_gc,
		    int dest_x, int dest_y, int dest_w, int dest_h)
{
	Pixmap tile_mask = None;
	XGCValues gcv;
	unsigned long gcm;
	GC my_gc = None;
	GC my_mono_gc = None;

	if (gc == None)
	{
		my_gc = fvwmlib_XCreateGC(dpy, d, 0, NULL);
	}
	if (mono_gc == None && (mask != None || Pdepth != depth))
	{
		if (mask != None)
			my_mono_gc = fvwmlib_XCreateGC(dpy, mask, 0, NULL);
		else if (depth != Pdepth)
			my_mono_gc = fvwmlib_XCreateGC(dpy, pixmap, 0, NULL);
	}
	gcm = 0;
	if (mask != None)
	{
		/* create a till mask */
		tile_mask = XCreatePixmap(dpy, win, dest_w, dest_h, 1);
		gcv.tile = mask;
		gcv.ts_x_origin = src_x;
		gcv.ts_y_origin = src_y;
		gcv.fill_style = FillTiled;
		gcm = GCFillStyle | GCClipXOrigin | GCClipYOrigin | GCTile;
		if (mono_gc != None)
		{
			XChangeGC(dpy, mono_gc, gcm, &gcv);
		}
		else
		{
			gcv.foreground = 1;
			gcv.background = 0;
			gcm |= GCBackground|GCForeground;
			XChangeGC(dpy, my_mono_gc, gcm, &gcv);
		}
		XFillRectangle(dpy, tile_mask,
			       (mono_gc != None)? mono_gc:my_mono_gc,
			       src_x, src_y, dest_w, dest_h);
		if (mono_gc != None)
		{
			gcv.fill_style = FillSolid;
			gcm = GCFillStyle;
			XChangeGC(dpy, mono_gc, gcm, &gcv);
		}
	}
	gcv.clip_mask = tile_mask;
	gcv.fill_style = FillTiled;
	gcv.tile = pixmap;
	gcv.ts_x_origin = src_x;
	gcv.ts_y_origin = src_y;
	gcm = GCFillStyle | GCClipMask | GCTile | GCTileStipXOrigin |
		GCTileStipYOrigin;
	if (depth != Pdepth)
	{
		Pixmap my_pixmap = None;

		XChangeGC(dpy,
			  (mono_gc != None)? mono_gc:my_mono_gc, gcm, &gcv);
		my_pixmap = XCreatePixmap(dpy, win, dest_w, dest_h, 1);
		XFillRectangle(dpy, my_pixmap,
			       (mono_gc != None)? mono_gc:my_mono_gc,
			       0, 0, dest_w, dest_h);
		gcv.clip_mask = my_pixmap;
		gcv.fill_style = FillSolid;
		gcm = GCFillStyle | GCClipMask;
		XChangeGC(dpy,
			  (mono_gc != None)? mono_gc:my_mono_gc,
			  gcm, &gcv);
		XCopyPlane(dpy, my_pixmap, d,
			  (my_gc != None)? my_gc:gc,
			  0, 0, dest_w, dest_h, dest_x, dest_y, 1);
		if (my_pixmap != None)
		{
			XFreePixmap(dpy, my_pixmap);
		}
	}
	else
	{
		XChangeGC(dpy, (gc != None)? gc:my_gc, gcm, &gcv);
		XFillRectangle(dpy, d,
			       (gc != None)? gc:my_gc,
			       dest_x, dest_y, dest_w, dest_h);
	}
	if (my_gc != None)
	{
		XFreeGC(dpy, my_gc);
	}
	else
	{
		gcv.clip_mask = None;
		gcv.fill_style = FillSolid;
		gcm = GCFillStyle | GCClipMask;
		XChangeGC(dpy, gc, gcm, &gcv);
	}
	if (my_mono_gc != None)
	{
		XFreeGC(dpy, my_mono_gc);
	}
	else if (mono_gc != None)
	{
		gcv.clip_mask = None;
		gcv.fill_style = FillSolid;
		gcm = GCFillStyle | GCClipMask;
		XChangeGC(dpy, mono_gc, gcm, &gcv);
	}
	if (tile_mask != None)
	{
		XFreePixmap(dpy, tile_mask);
	}
}

static
Pixmap PCreateTintedPixmap(
	Display *dpy, Window win, int percent, Pixel tint,
	Drawable src, Pixmap mask, int depth, GC gc, int color_limit,
	int x, int y, int width, int height)
{
	XImage *src_im;
	XImage *mask_im = NULL;
	XImage *out_im;
	GC my_gc;
	Pixmap out_pix = None;
	unsigned char *cm;
	XColor *colors;
	XColor tint_color, c;
	int j, i, m = 0, k = 0;

	if (depth < 2 || depth != Pdepth)
		return None;

	if (!(src_im =
	      XGetImage(dpy, src, x, y, width, height, AllPlanes, ZPixmap)))
	{
		return None;
	}
	if (mask != None)
	{
		mask_im = XGetImage(
			dpy, mask, 0, 0, width, height, AllPlanes, ZPixmap);
	}
	out_pix = XCreatePixmap(dpy, win, width, height, Pdepth);
	out_im = XCreateImage(
		dpy, Pvisual, Pdepth, ZPixmap, 0, 0, width, height,
		Pdepth > 16 ? 32 : (Pdepth > 8 ? 16 : 8), 0);
	if (gc == None)
	{
		my_gc = fvwmlib_XCreateGC(dpy, win, 0, NULL);
	}
	else
	{
		my_gc = gc;
	}

	if (!out_pix || !out_im || !my_gc)
	{
		XDestroyImage(src_im);
		if (mask_im)
		{
			XDestroyImage(mask_im);
		}
		if (out_pix)
		{
			XFreePixmap(dpy, out_pix);
		}
		if (out_im)
		{
			XDestroyImage(out_im);
		}
		if (gc == None && my_gc)
		{
			XFreeGC(dpy, my_gc);
		}
		return None;
	}

	colors = (XColor *)safemalloc(width * height * sizeof(XColor));
	cm = (unsigned char *)safemalloc(width * height * sizeof(char));
	out_im->data = safemalloc(out_im->bytes_per_line * height);

	tint_color.pixel = tint;
	XQueryColor(dpy, Pcmap, &tint_color);

	for (j = 0; j < height; j++)
	{
		for (i = 0; i < width; i++)
		{
			if (mask_im != NULL &&
			    (XGetPixel(mask_im, i, j) == 0))
			{
				cm[m++] = 0;
			}
			else
			{
				cm[m++] = 255;
				colors[k++].pixel = XGetPixel(src_im, i, j);
			}
		}
	}

	for (i = 0; i < k; i += 256)
		XQueryColors(dpy, Pcmap, &colors[i], min(k - i, 256));

	k = 0;m = 0;
	c.flags = DoRed | DoGreen | DoBlue;
	for (j = 0; j < height; j++)
	{
		for (i = 0; i < width; i++)
		{
			if (cm[m] > 0)
			{
				c.blue = (unsigned short)
					(((100-percent)*colors[k].blue +
					  tint_color.blue * percent) / 100);
				c.green = (unsigned short)
					(((100-percent)*colors[k].green +
					  tint_color.green * percent) / 100);
				c.red = (unsigned short)
					(((100-percent)*colors[k].red +
					  tint_color.red * percent) / 100);
				k++;
				if (Pvisual->class == DirectColor ||
				    Pvisual->class == TrueColor)
				{
					c.pixel = PictureRGBtoPixel(
						c.red/257,
						c.green/257,
						c.blue/257);
				}
				else
				{

					PictureReduceRGBColor(&c,color_limit);
				}
			}
			else
			{
				c.pixel = XGetPixel(src_im, i, j);
			}
			XPutPixel(out_im, i, j, c.pixel);
			m++;
		}
	}
	free(colors);
	free(cm);
	XDestroyImage(src_im);
	if (mask_im)
	{
		XDestroyImage(mask_im);
	}
	XPutImage(dpy, out_pix, my_gc, out_im, 0, 0, 0, 0, width, height);
	if (gc == None)
	{
		XFreeGC(dpy, my_gc);
	}
	XDestroyImage(out_im);

	return out_pix;
}

/* ---------------------------- interface functions ------------------------- */

void PGraphicsCopyPixmaps(Display *dpy,
			  Pixmap pixmap, Pixmap mask, Pixmap alpha,
			  int depth, Drawable d, GC gc,
			  int src_x, int src_y, int src_w, int src_h,
			  int dest_x, int dest_y)
{

	if (!XRenderSupport || alpha == None || !FRenderGetExtensionSupported())
	{
		PCopyArea(dpy, pixmap, mask, depth,
			  d, gc, src_x, src_y, src_w,src_h,
			  dest_x, dest_y);
		return;
	}
	else
	{
		FRenderCopyArea(dpy,  pixmap, mask, alpha,
				d, src_x, src_y,
				dest_x, dest_y, src_w, src_h, False);
	}
}

void PGraphicsCopyFvwmPicture(Display *dpy, FvwmPicture *p, Drawable d, GC gc,
			      int src_x, int src_y, int src_w, int src_h,
			      int dest_x, int dest_y)
{

	if (!XRenderSupport || p->alpha == None ||
	    !FRenderGetExtensionSupported())
	{
		PCopyArea(dpy, p->picture, p->mask, p->depth,
			  d, gc, src_x, src_y, src_w,src_h,
			  dest_x, dest_y);
		return;
	}
	else
	{
		FRenderCopyArea(dpy, p->picture, p->mask, p->alpha, d,
				src_x, src_y,
				dest_x, dest_y, src_w, src_h, False);
	}
}

void PGraphicsTileRectangle(Display *dpy, Window win,
			    Pixmap pixmap, Pixmap mask, Pixmap alpha,
			    int depth,
			    int src_x, int src_y,
			    Drawable d, GC gc, GC mono_gc,
			    int dest_x, int dest_y, int dest_w, int dest_h)
{
	if (!XRenderSupport || alpha == None || !FRenderGetExtensionSupported())
	{
		PTileRectangle(dpy, win, pixmap, mask, depth,
			       src_x, src_y,
			       d, gc, mono_gc, dest_x, dest_y, dest_w, dest_h);
		return;
	}
	else
	{
		FRenderCopyArea(dpy, pixmap, mask, alpha, d,
				src_x, src_y,
				dest_x, dest_y, dest_w, dest_h, True);
	}
}

void
PGraphicsTintRectangle(
	Display *dpy, Window win, int tint_percent, Pixel tint,
	Drawable d, Pixmap mask, int depth, GC gc, int color_limit,
	int dest_x, int dest_y, int dest_w, int dest_h)
{
	if (!XRenderSupport || !FRenderGetExtensionSupported())
	{
		/* FIXME: work only if d is a pixmap */
		Pixmap tinted_pix;

		tinted_pix = PCreateTintedPixmap(
			dpy, win, tint_percent, tint, (Pixmap)d, mask,
			depth, gc, color_limit, dest_x, dest_y, dest_w, dest_h);
		if (tinted_pix == None)
		{
			return;
		}
		PCopyArea(
			dpy, tinted_pix, None, depth, d, None,
			0, 0, dest_w, dest_h, dest_x, dest_y);
		XFreePixmap(dpy, tinted_pix);
			
	}
	else
	{
		FRenderTintRectangle(dpy, win, tint_percent, tint, mask, d,
				     dest_x, dest_y, dest_w, dest_h);
	}
}
