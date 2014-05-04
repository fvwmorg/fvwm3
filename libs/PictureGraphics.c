/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis */
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

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>

#include <X11/Xlib.h>

#include <fvwmlib.h>
#include "PictureBase.h"
#include "Colorset.h"
#include "FRenderInit.h"
#include "FRenderInterface.h"
#include "Graphics.h"
#include "PictureGraphics.h"
#include "PictureUtils.h"
#include "FImage.h"
#include "Grab.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static Bool PGrabImageError = True;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */
static
int FSetBackingStore(Display *dpy, Window win, int backing_store)
{
	XWindowAttributes attributes;
	XSetWindowAttributes set_attributes;
	int old_bs;

	XGetWindowAttributes(dpy, win, &attributes);
	if (attributes.backing_store == backing_store)
	{
		return -1;
	}
	old_bs = attributes.backing_store;
	set_attributes.backing_store = backing_store;
	XChangeWindowAttributes(dpy, win, CWBackingStore, &set_attributes);
	return old_bs;
}

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
	gcv.clip_x_origin = dest_x - src_x; /* */
	gcv.clip_y_origin = dest_y - src_y; /* */
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
			  src_x, src_y, src_w, src_h,
			  dest_x, dest_y);
	}
	else
	{
		/* monochrome bitmap */
		gcv.clip_mask = mask;
		if (my_gc != None)
		{
			gcv.foreground = PictureWhitePixel();
			gcv.background = PictureBlackPixel();
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
		gcm = GCFillStyle | GCTileStipXOrigin | GCTileStipYOrigin |
			GCTile;
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

	gcv.tile = pixmap;
	gcv.ts_x_origin = dest_x - src_x;
	gcv.ts_y_origin = dest_y - src_y;
	gcv.fill_style = FillTiled;
	gcm = GCFillStyle | GCTile | GCTileStipXOrigin | GCTileStipYOrigin;
	gcv.clip_mask = tile_mask;
	gcv.clip_x_origin = dest_x;
	gcv.clip_y_origin = dest_y;;
	gcm |= GCClipMask | GCClipXOrigin | GCClipYOrigin;
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
void PGrabImageErrorHandler(void)
{
	PGrabImageError = True;
}

static
FImage *PGrabXImage(
	Display *dpy, Drawable d, int x, int y, int w, int h, Bool d_is_a_window)
{

	Bool try_to_grab = True;
	XWindowAttributes   xwa;
	XErrorHandler saved_eh = NULL;
	FImage *fim = NULL;

	PGrabImageError = 0;
	if (d_is_a_window)
	{
		MyXGrabServer(dpy);
		XGetWindowAttributes(dpy, d, &xwa);
		XSync(dpy, False);

		if (xwa.map_state != IsViewable &&
		    xwa.backing_store == NotUseful)
		{
			try_to_grab = False;
#if 0
			fprintf(stderr, "Bad attribute! %i,%i\n",
				xwa.map_state != IsViewable,
				xwa.backing_store == NotUseful);
#endif
		}
		else
		{
			saved_eh = XSetErrorHandler(
				(XErrorHandler) PGrabImageErrorHandler);
#if 0
			fprintf(stderr, "Attribute ok! %i,%i\n",
				xwa.map_state != IsViewable,
				xwa.backing_store == NotUseful);
#endif
		}
	}
	if (try_to_grab)
	{
		fim = FGetFImage(dpy, d, Pvisual, Pdepth, x, y, w, h, AllPlanes,
				 ZPixmap);
		if (PGrabImageError)
		{
#if 0
			fprintf(stderr, "XGetImage error during the grab\n");
#endif
			if (fim != NULL)
			{
				FDestroyFImage(dpy, fim);
				fim = NULL;
			}
		}
		if (d_is_a_window)
		{
			XSetErrorHandler((XErrorHandler) saved_eh);
		}
	}

	if (d_is_a_window)
	{
		MyXUngrabServer(dpy);
	}
	return fim;
}

static
Pixmap PCreateRenderPixmap(
	Display *dpy, Window win, Pixmap pixmap, Pixmap mask, Pixmap alpha,
	int depth, int added_alpha_percent, Pixel tint, int tint_percent,
	Bool d_is_a_window, Drawable d, GC gc, GC mono_gc, GC alpha_gc,
	int src_x, int src_y, int src_w, int src_h,
	int dest_x, int dest_y, int dest_w, int dest_h, Bool do_repeat,
	int *new_w, int *new_h, Bool *new_do_repeat,
	Pixmap *new_mask)
{
	FImage *pixmap_fim = NULL;
	FImage *mask_fim = NULL;
	FImage *alpha_fim = NULL;
	FImage *dest_fim = NULL;
	FImage *new_mask_fim = NULL;
	FImage *out_fim = NULL;
	Pixmap pixmap_copy = None;
	Pixmap src_pix = None;
	Pixmap out_pix = None;
	unsigned short *am = NULL;
	XColor *colors = NULL, *dest_colors = NULL;
	XColor tint_color, c;
	int w ,h, n_src_w, n_src_h;
	int j, i, j1, i1, m = 0, k = 0, l = 0;
	Bool do_free_mono_gc = False;
	Bool make_new_mask = False;
	Bool error = False;

	*new_mask = None;
	*new_do_repeat = do_repeat;

	if (depth != Pdepth)
	{
		pixmap_copy = XCreatePixmap(dpy, win, src_w, src_h, Pdepth);
		if (gc == None)
		{
			gc = PictureDefaultGC(dpy, win);
		}
		if (pixmap_copy && gc)
		{
			XCopyPlane(
				dpy, pixmap, pixmap_copy, gc,
				src_x, src_y, src_w, src_h,
				0, 0, 1);
		}
		else
		{
			error = True;
			goto bail;
		}
		src_x = src_y = 0;
	}
	src_pix = (pixmap_copy)? pixmap_copy:pixmap;

	if (src_pix == ParentRelative)
	{
		pixmap_fim = PGrabXImage(
			dpy, d, dest_x, dest_y, dest_w, dest_h, d_is_a_window);
	}
	else
	{
		pixmap_fim = FGetFImage(
			dpy, src_pix, Pvisual, Pdepth, src_x, src_y, src_w,
			src_h, AllPlanes, ZPixmap);
	}
	if (!pixmap_fim)
	{
		error = True;
		goto bail;
	}
	if (mask != None)
	{
		mask_fim = FGetFImage(
			dpy, mask, Pvisual, 1, src_x, src_y, src_w, src_h,
			AllPlanes, ZPixmap);
		if (!mask_fim)
		{
			error = True;
			goto bail;
		}
		if (src_x != 0 || src_y != 0)
			make_new_mask = True;
	}
	if (alpha != None)
	{
		alpha_fim = FGetFImage(
			dpy, alpha, Pvisual, FRenderGetAlphaDepth(), src_x,
			src_y, src_w, src_h, AllPlanes, ZPixmap);
		if (!alpha_fim)
		{
			error = True;
			goto bail;
		}
	}

	if (alpha != None || added_alpha_percent < 100)
	{
		dest_fim = PGrabXImage(
			dpy, d, dest_x, dest_y, dest_w, dest_h, d_is_a_window);
		/* accept this error */
	}

	if (dest_fim && do_repeat && (dest_w > src_w || dest_h > src_h))
	{
		*new_do_repeat = False;
		if (mask)
		{
			make_new_mask = True;
		}
		w = dest_w;
		h = dest_h;
		n_src_w = (w < src_w)? w:src_w;
		n_src_h = (h < src_h)? h:src_h;
	}
	else
	{
		n_src_w = w = (dest_w < src_w)? dest_w:src_w;
		n_src_h = h = (dest_h < src_h)? dest_h:src_h;
	}
	*new_w = w;
	*new_h = h;

	out_pix = XCreatePixmap(dpy, win, w, h, Pdepth);
	out_fim = FCreateFImage(
		dpy, Pvisual, Pdepth, ZPixmap, w, h);
	if (gc == None)
	{
		gc = PictureDefaultGC(dpy, win);
	}

	if (!out_pix || !out_fim || !gc)
	{
		error = True;
		goto bail;
	}

	colors = xmalloc(n_src_w * n_src_h * sizeof(XColor));
	if (dest_fim)
	{
		dest_colors = xmalloc(w * h * sizeof(XColor));
	}
	am = xmalloc(n_src_w * n_src_h * sizeof(unsigned short));

	if (tint_percent > 0)
	{
		tint_color.pixel = tint;
		XQueryColor(dpy, Pcmap, &tint_color);
	}

	for (j = 0; j < n_src_h; j++)
	{
		for (i = 0; i < n_src_w; i++, m++)
		{
			if (mask_fim != NULL &&
			    (XGetPixel(mask_fim->im, i, j) == 0))
			{
				am[m] = 0;
			}
			else if (alpha_fim != NULL)
			{
				am[m] = XGetPixel(alpha_fim->im, i, j);
				if (am[m] == 0 && !dest_fim)
				{
					make_new_mask = True;
				}
			}
			else
			{
				am[m] = 255;
			}
			if (added_alpha_percent < 100)
			{
				am[m] = (unsigned short)
					((am[m] * added_alpha_percent) / 100);
			}
			if (am[m] > 0)
			{
				if (!dest_fim)
				{
					if (am[m] < 130)
					{
						am[m] = 0;
						make_new_mask = True;
					}
					else
					{
						am[m] = 255;
					}
				}
				else if (am[m] < 255)
				{
					dest_colors[l++].pixel =
						XGetPixel(dest_fim->im, i, j);
				}
				if (am[m] > 0)
				{
					colors[k++].pixel =
						XGetPixel(pixmap_fim->im, i, j);
				}
			}
		}
	}

	for (i = 0; i < k; i += 256)
		XQueryColors(dpy, Pcmap, &colors[i], min(k - i, 256));

	if (do_repeat && dest_fim && (n_src_h < h || n_src_w < w))
	{
		for (j1 = 0; j1 < h+n_src_h; j1 +=n_src_h)
		{
			for (i1 = 0; i1 < w+n_src_w; i1 += n_src_w)
			{
				for(j = 0;
				    !(i1==0 && j1==0) && j < n_src_h && j+j1 < h;
				    j++)
				{
					for(i = 0; i < n_src_w && i+i1 < w; i++)
					{
						m = j*n_src_w + i;
						if (am[m] > 0 && am[m] < 255)
						{
							dest_colors[l++].pixel =
								XGetPixel(
									dest_fim
									->im,
									i1+i,
									j1+j);
						}
					}
				}
			}
		}
	}

	for (i = 0; i < l; i += 256)
		XQueryColors(dpy, Pcmap, &dest_colors[i], min(l - i, 256));

	if (make_new_mask)
	{
		 *new_mask = XCreatePixmap(dpy, win, w, h, 1);
		 if (*new_mask)
		 {
			 new_mask_fim = FCreateFImage(
				 dpy, Pvisual, 1, ZPixmap, w, h);
			 if (mono_gc == None)
			 {
				 mono_gc = fvwmlib_XCreateGC(
					 dpy, *new_mask, 0, NULL);
				 do_free_mono_gc = True;
			 }
		 }
	}

	l = 0; m = 0; k = 0;
	c.flags = DoRed | DoGreen | DoBlue;
	for (j = 0; j < n_src_h; j++)
	{
		for (i = 0; i < n_src_w; i++, m++)
		{
			if (am[m] > 0)
			{
				if (*new_mask)
				{
					XPutPixel(new_mask_fim->im, i, j, 1);
				}
				if (tint_percent > 0)
				{
					colors[k].blue = (unsigned short)
						(((100-tint_percent)*
						  colors[k].blue +
						  tint_color.blue *
						  tint_percent) /
						 100);
					colors[k].green = (unsigned short)
						(((100-tint_percent)*
						  colors[k].green +
						  tint_color.green *
						  tint_percent) /
						 100);
					colors[k].red = (unsigned short)
						(((100-tint_percent)*
						  colors[k].red +
						  tint_color.red *
						  tint_percent) /
						 100);
				}
				c.blue = colors[k].blue;
				c.green = colors[k].green;
				c.red = colors[k].red;
				if (am[m] < 255 && dest_fim)
				{
					c.blue = (unsigned short)
						(((255 - am[m])*
						  dest_colors[l].blue +
						  c.blue * am[m]) /
						 255);
					c.green = (unsigned short)
						(((255 - am[m])*
						  dest_colors[l].green +
						  c.green * am[m]) /
						 255);
					c.red = (unsigned short)
						(((255 - am[m])*
						  dest_colors[l].red +
						  c.red * am[m]) /
						 255);
					l++;
				}
				PictureAllocColor(Pdpy, Pcmap, &c, False);
				colors[k].pixel = c.pixel;
				k++;
			}
			else
			{
				if (dest_fim)
				{
					c.pixel = XGetPixel(dest_fim->im, i, j);
				}
				else
				{
					c.pixel = XGetPixel(
						pixmap_fim->im, i, j);
				}
				if (*new_mask)
				{
					XPutPixel(new_mask_fim->im, i, j, 0);
				}
			}
			XPutPixel(out_fim->im, i, j, c.pixel);
		}
	}

	/* tile: editor ligne width limit  107 !!*/
	if (do_repeat && dest_fim && (n_src_h < h || n_src_w < w))
	{
		for (j1 = 0; j1 < h+n_src_h; j1 +=n_src_h)
		{
			for (i1 = 0; i1 < w+n_src_w; i1 += n_src_w)
			{
				k = 0;
				for(j = 0;
				    !(i1==0 && j1==0) && j < n_src_h; j++)
				{
					for(i = 0; i < n_src_w; i++)
					{
						m = j*n_src_w + i;
						if (!(i+i1 < w && j+j1 < h))
						{
							if (am[m] > 0)
							{
								k++;
							}
						}
						else
						{
							if (am[m] > 0)
							{
								if (*new_mask)
								{
									XPutPixel(
										new_mask_fim->im, i+i1,
										j+j1, 1);
								}
								c.blue = colors[k].blue;
								c.green = colors[k].green;
								c.red = colors[k].red;
								c.pixel = colors[k].pixel;
								k++;
								if (am[m] < 255)
								{
									c.blue = (unsigned short)
										(((255 - am[m])*
										  dest_colors[l].blue +
										  c.blue * am[m]) /
										 255);
									c.green = (unsigned short)
										(((255 - am[m])*
										  dest_colors[l].green +
										  c.green * am[m]) /
										 255);
									c.red = (unsigned short)
										(((255 - am[m])*
										  dest_colors[l].red +
										  c.red * am[m]) /
										 255);
									l++;
									PictureAllocColor(
										Pdpy, Pcmap, &c, False);
								}
							}
							else
							{
								c.pixel = XGetPixel(
									dest_fim->im, i+i1, j+j1);
								if (*new_mask)
								{
									XPutPixel(
										new_mask_fim->im, i+i1,
										j+j1, 0);
								}
							}
							XPutPixel(out_fim->im, i+i1, j+j1, c.pixel);
						}
					}
				}
			}
		}
	}

	FPutFImage(dpy, out_pix, gc, out_fim, 0, 0, 0, 0, w, h);
	if (*new_mask && mono_gc)
	{
		FPutFImage(
			dpy, *new_mask, mono_gc, new_mask_fim,
			0, 0, 0, 0, w, h);
	}

 bail:
	if (colors)
	{
		free(colors);
	}
	if (dest_colors)
	{
		free(dest_colors);
	}
	if (am)
	{
		free(am);
	}
	if (pixmap_copy)
	{
		XFreePixmap(dpy, pixmap_copy);
	}
	if (pixmap_fim)
	{
		FDestroyFImage(dpy, pixmap_fim);
	}
	if (mask_fim)
	{
		FDestroyFImage(dpy, mask_fim);
	}
	if (alpha_fim)
	{
		FDestroyFImage(dpy, alpha_fim);
	}
	if (dest_fim)
	{
		FDestroyFImage(dpy, dest_fim);
	}
	if (new_mask_fim)
	{
		FDestroyFImage(dpy, new_mask_fim);
	}
	if (do_free_mono_gc && mono_gc)
	{
		XFreeGC(dpy, mono_gc);
	}
	if (out_fim)
	{
		FDestroyFImage(dpy, out_fim);
	}
	if (error)
	{
		if (out_pix != None)
		{
			XFreePixmap(dpy, out_pix);
			out_pix = None;
		}
		if (*new_mask != None)
		{
			XFreePixmap(dpy, *new_mask);
			*new_mask = None;
		}
	}

	return out_pix;
}

/* never used and tested */
static
Pixmap PCreateDitherPixmap(
	Display *dpy, Window win, Drawable src, Pixmap mask, int depth, GC gc,
	int in_width, int in_height, int out_width, int out_height)
{
	FImage *src_fim;
	FImage *mask_fim = NULL;
	FImage *out_fim;
	Pixmap out_pix = None;
	unsigned char *cm;
	XColor *colors;
	XColor c;
	int j, i, m = 0, k = 0, x = 0, y = 0;

	if (depth != Pdepth)
		return None;

	if (!(src_fim =
	      FGetFImage(
		      dpy, src, Pvisual, depth, 0, 0, in_width, in_height,
		      AllPlanes, ZPixmap)))
	{
		return None;
	}
	if (mask != None)
	{
		mask_fim = FGetFImage(
			dpy, mask, Pvisual, 1, 0, 0, in_width, in_height,
			AllPlanes, ZPixmap);
		if (!mask_fim)
		{
			FDestroyFImage(dpy, mask_fim);
			return None;
		}
	}
	out_pix = XCreatePixmap(dpy, win, out_width, out_height, Pdepth);
	out_fim = FCreateFImage(
		dpy, Pvisual, Pdepth, ZPixmap, out_width, out_height);
	if (gc == None)
	{
		gc = PictureDefaultGC(dpy, win);
	}

	if (!out_pix || !out_fim || !gc)
	{
		FDestroyFImage(dpy, src_fim);
		if (mask_fim)
		{
			FDestroyFImage(dpy, mask_fim);
		}
		if (out_pix)
		{
			XFreePixmap(dpy, out_pix);
		}
		if (out_fim)
		{
			FDestroyFImage(dpy, out_fim);
		}
		return None;
	}

	colors = xmalloc(out_width * out_height * sizeof(XColor));
	cm = xmalloc(out_width * out_height * sizeof(char));

	x = y = 0;
	for (j = 0; j < out_height; j++,y++)
	{
		if (y == in_height)
			y = 0;
		for (i = 0; i < out_width; i++,x++)
		{
			if (x == in_width)
				x = 0;
			if (mask_fim != NULL &&
			    (XGetPixel(mask_fim->im, x, y) == 0))
			{
				cm[m++] = 0;
			}
			else
			{
				cm[m++] = 255;
				colors[k++].pixel = XGetPixel(src_fim->im, x, y);
			}
		}
	}

	for (i = 0; i < k; i += 256)
		XQueryColors(dpy, Pcmap, &colors[i], min(k - i, 256));

	k = 0;m = 0;
	for (j = 0; j < out_height; j++)
	{
		for (i = 0; i < out_width; i++)
		{

			if (cm[m] > 0)
			{
				c = colors[k++];
				PictureAllocColorAllProp(
					Pdpy, Pcmap, &c, i, j, False, False,
					True);
			}
			else
			{
				c.pixel = XGetPixel(src_fim->im, i, j);
			}
			XPutPixel(out_fim->im, i, j, c.pixel);
			m++;
		}
	}
	free(colors);
	free(cm);
	FDestroyFImage(dpy, src_fim);
	if (mask_fim)
	{
		FDestroyFImage(dpy, mask_fim);
	}
	FPutFImage(
		dpy, out_pix, gc, out_fim, 0, 0, 0, 0, out_width, out_height);
	FDestroyFImage(dpy, out_fim);

	return out_pix;
}

/* ---------------------------- interface functions ------------------------ */

Pixmap PictureBitmapToPixmap(
	Display *dpy, Window win, Pixmap src, int depth, GC gc,
	int src_x, int src_y, int src_w, int src_h)
{
	Pixmap dest = None;

	dest = XCreatePixmap(dpy, win, src_w, src_h, depth);
	if (dest && gc == None)
	{
		gc = PictureDefaultGC(dpy, win);
	}
	if (dest && gc)
	{
		XCopyPlane(
			dpy, src, dest, gc,
			src_x, src_y, src_w, src_h, 0, 0, 1);
	}

	return dest;
}

void PGraphicsRenderPixmaps(
	Display *dpy, Window win, Pixmap pixmap, Pixmap mask, Pixmap alpha,
	int depth, FvwmRenderAttributes *fra, Drawable d,
	GC gc, GC mono_gc, GC alpha_gc,
	int src_x, int src_y, int src_w, int src_h,
	int dest_x, int dest_y, int dest_w, int dest_h, int do_repeat)
{
	FvwmRenderAttributes t_fra;
	Pixmap xrs_pixmap = None;
	Pixmap xrs_mask = None;
	Pixmap tmp_pixmap, tmp_mask;
	Bool d_is_a_window;

	t_fra.added_alpha_percent = 100;
	t_fra.tint_percent = 0;
	t_fra.mask = 0;
	t_fra.tint = None;

	if (fra)
	{
		t_fra.mask = fra->mask;
		if (fra->mask & FRAM_HAVE_ICON_CSET)
		{
			t_fra.added_alpha_percent =
				fra->colorset->icon_alpha_percent;
			t_fra.tint_percent = fra->colorset->icon_tint_percent;
			t_fra.tint = fra->colorset->icon_tint;
		}
		if (fra->mask & FRAM_HAVE_ADDED_ALPHA)
		{
			t_fra.added_alpha_percent = fra->added_alpha_percent;
		}
		if (fra->mask & FRAM_HAVE_TINT)
		{
			t_fra.tint_percent = fra->tint_percent;
			t_fra.tint = fra->tint;
		}
	}
	if (dest_w == 0 && dest_h == 0)
	{
		dest_w = src_w; dest_h = src_h;
	}

	/* use XRender only when "needed" (backing store pbs) */
	if (t_fra.tint_percent > 0 || t_fra.added_alpha_percent < 100
	    || alpha != None)
	{
		/* for testing XRender simulation add && 0 */
		if (FRenderRender(
			dpy, win, pixmap, mask, alpha, depth,
			t_fra.added_alpha_percent, t_fra.tint,
			t_fra.tint_percent,
			d, gc, alpha_gc, src_x, src_y, src_w, src_h,
			dest_x, dest_y, dest_w, dest_h, do_repeat))
		{
			return;
		}
	}

	/* no render extension or something strange happen */
	if (t_fra.tint_percent > 0 || t_fra.added_alpha_percent < 100
	    || alpha != None)
	{
		int new_w, new_h, new_do_repeat;

		d_is_a_window = !!(t_fra.mask & FRAM_DEST_IS_A_WINDOW);
		xrs_pixmap = PCreateRenderPixmap(
			dpy, win, pixmap, mask, alpha, depth,
			t_fra.added_alpha_percent, t_fra.tint,
			t_fra.tint_percent, d_is_a_window, d,
			gc, mono_gc, alpha_gc,
			src_x, src_y, src_w, src_h,
			dest_x, dest_y, dest_w, dest_h, do_repeat,
			&new_w, &new_h, &new_do_repeat, &xrs_mask);
		if (xrs_pixmap)
		{
			src_x = 0;
			src_y = 0;
			src_w = new_w;
			src_h = new_h;
			depth = Pdepth;
			do_repeat = new_do_repeat;
		}
	}
	tmp_pixmap = (xrs_pixmap != None)? xrs_pixmap:pixmap;
	tmp_mask = (xrs_mask != None)? xrs_mask:mask;
	if (do_repeat)
	{
		PTileRectangle(
			dpy, win, tmp_pixmap, tmp_mask, depth,
			src_x, src_y, d, gc, mono_gc, dest_x, dest_y, dest_w,
			dest_h);
	}
	else
	{
		PCopyArea(
			dpy, tmp_pixmap, tmp_mask, depth, d, gc,
			src_x, src_y, src_w, src_h, dest_x, dest_y);
	}
	if (xrs_pixmap)
	{
		XFreePixmap(dpy, xrs_pixmap);
	}
	if (xrs_mask)
	{
		XFreePixmap(dpy, xrs_mask);
	}
}

void PGraphicsRenderPicture(
	Display *dpy, Window win, FvwmPicture *p, FvwmRenderAttributes *fra,
	Drawable d, GC gc, GC mono_gc, GC alpha_gc,
	int src_x, int src_y, int src_w, int src_h,
	int dest_x, int dest_y, int dest_w, int dest_h, int do_repeat)
{
	PGraphicsRenderPixmaps(
		dpy, win, p->picture, p->mask, p->alpha, p->depth, fra,
		d, gc, mono_gc, alpha_gc,
		src_x, src_y, src_w, src_h,
		dest_x, dest_y, dest_w, dest_h, do_repeat);
}

void PGraphicsCopyPixmaps(
	Display *dpy, Pixmap pixmap, Pixmap mask, Pixmap alpha,
	int depth, Drawable d, GC gc, int src_x, int src_y, int src_w, int src_h,
	int dest_x, int dest_y)
{
	PGraphicsRenderPixmaps(
		dpy,  None, pixmap, mask, alpha, depth, 0, d, gc, None, None,
		src_x, src_y, src_w, src_h, dest_x, dest_y, src_w, src_h, False);
}

void PGraphicsCopyFvwmPicture(
	Display *dpy, FvwmPicture *p, Drawable d, GC gc,
	int src_x, int src_y, int src_w, int src_h, int dest_x, int dest_y)
{
	PGraphicsRenderPicture(
		dpy, None, p, 0, d, gc, None, None, src_x, src_y, src_w, src_h,
		dest_x, dest_y, src_w, src_h, False);
}

void PGraphicsTileRectangle(
	Display *dpy, Window win, Pixmap pixmap, Pixmap mask, Pixmap alpha,
	int depth, Drawable d, GC gc, GC mono_gc,
	int src_x, int src_y,  int src_w, int src_h,
	int dest_x, int dest_y, int dest_w, int dest_h)
{
	PGraphicsRenderPixmaps(
		dpy, win, pixmap, mask, alpha, depth, 0, d, gc, mono_gc, None,
		src_x, src_y, dest_w, dest_h, dest_x, dest_y, dest_w, dest_h,
		True);
}

FvwmPicture *PGraphicsCreateStretchPicture(
	Display *dpy, Window win, FvwmPicture *src,
	int dest_width, int dest_height, GC gc, GC mono_gc, GC alpha_gc)
{
	Pixmap pixmap = None, mask = None, alpha = None;
	FvwmPicture *q;

	if (src == NULL || src->picture == None)
	{
		return NULL;
	}
	pixmap = CreateStretchPixmap(
		dpy, src->picture, src->width, src->height, src->depth,
		dest_width, dest_height, gc);
	if (!pixmap)
	{
		return NULL;
	}
	if (src->mask)
	{
		mask = CreateStretchPixmap(
			dpy, src->mask, src->width, src->height, 1,
			dest_width, dest_height, mono_gc);
	}
	if (src->alpha)
	{
		alpha = CreateStretchPixmap(
			dpy, src->alpha, src->width, src->height,
			FRenderGetAlphaDepth(),
			dest_width, dest_height, alpha_gc);
	}

	q = xcalloc(1, sizeof(FvwmPicture));
	q->count = 1;
	q->name = NULL;
	q->next = NULL;
	q->stamp = pixmap;
	q->picture = pixmap;
	q->mask = mask;
	q->alpha = alpha;
	q->width = dest_width;
	q->height = dest_height;
	q->depth = src->depth;
	q->alloc_pixels = 0;
	q->nalloc_pixels = 0;

	return q;
}

FvwmPicture *PGraphicsCreateTiledPicture(
	Display *dpy, Window win, FvwmPicture *src,
	int dest_width, int dest_height, GC gc, GC mono_gc, GC alpha_gc)
{
	Pixmap pixmap = None, mask = None, alpha = None;
	FvwmPicture *q;

	if (src == NULL || src->picture == None)
	{
		return NULL;
	}
	pixmap = CreateTiledPixmap(
		dpy, src->picture, src->width, src->height, dest_width,
		dest_height, src->depth, gc);
	if (!pixmap)
	{
		return NULL;
	}
	if (src->mask)
	{
		mask = CreateTiledPixmap(
			dpy, src->mask, src->width, src->height, dest_width,
			dest_height, 1, mono_gc);
	}
	if (src->alpha)
	{
		alpha = CreateTiledPixmap(
			dpy, src->alpha, src->width, src->height, dest_width,
			dest_height, FRenderGetAlphaDepth(), alpha_gc);
	}

	q = xcalloc(1, sizeof(FvwmPicture));
	q->count = 1;
	q->name = NULL;
	q->next = NULL;
	q->stamp = pixmap;
	q->picture = pixmap;
	q->mask = mask;
	q->alpha = alpha;
	q->width = dest_width;
	q->height = dest_height;
	q->depth = src->depth;
	q->alloc_pixels = 0;
	q->nalloc_pixels = 0;

	return q;
}

Pixmap PGraphicsCreateTransparency(
	Display *dpy, Window win, FvwmRenderAttributes *fra, GC gc,
	int x, int y, int width, int height, Bool parent_relative)
{
	Pixmap r = None, dp = None;
	XID junk;
	XID root;
	int dummy, sx, sy, sw, sh;
	int gx = x, gy = y, gh = height, gw = width;
	int old_backing_store = -1;

	if (parent_relative)
	{
		old_backing_store = FSetBackingStore(dpy, win, Always);
		XSetWindowBackgroundPixmap(dpy, win, ParentRelative);
		XClearArea(dpy, win, x, y, width, height, False);
		XSync(dpy, False);
	}

	if (parent_relative)
	{
		/* this block is not useful if backing store ... */
		if (!XGetGeometry(
			dpy, win, &root, (int *)&junk, (int *)&junk,
			(unsigned int *)&sw, (unsigned int *)&sh,
			(unsigned int *)&junk, (unsigned int *)&junk))
		{
			goto bail;
		}
		XTranslateCoordinates(
			dpy, win, DefaultRootWindow(dpy), x, y, &sx, &sy, &junk);
		if (sx >= DisplayWidth(dpy, DefaultScreen(dpy)))
		{
			goto bail;
		}
		if (sy >= DisplayHeight(dpy, DefaultScreen(dpy)))
		{
			goto bail;
		}
		if (sx < 0)
		{
			gx = gx - sx;
			gw = width + sx;
			sx = 0;
			if (gw <= 0)
			{
				goto bail;
			}
		}
		if (sy < 0)
		{
			gy = gy - sy;
			gh = height + sy;
			sy = 0;
			if (gh <= 0)
			{
				goto bail;
			}
		}
		if (sx + gw > DisplayWidth(dpy, DefaultScreen(dpy)))
		{
			gw = DisplayWidth(dpy, DefaultScreen(dpy)) - sx;
		}
		if (sy + gh > DisplayHeight(dpy, DefaultScreen(dpy)))
		{
			gh = DisplayHeight(dpy, DefaultScreen(dpy)) - sy;
		}
	}
#if 0
	fprintf(
		stderr,"Geo: %i,%i,%i,%i / %i,%i,%i,%i / %i,%i,%i,%i\n",
		gx,gy,gw,gh, x,y,width,height, sx,sy,sw,sh);
#endif
	if (XRenderSupport && FRenderGetExtensionSupported())
	{
		r = XCreatePixmap(dpy, win, gw, gh, Pdepth);
		if (FRenderRender(
			dpy, win, ParentRelative, None, None, Pdepth, 100,
			fra->tint, fra->tint_percent, r, gc, None,
			gx, gy, gw, gh, 0, 0, gw, gh, False))
		{
			goto bail;
		}
		XFreePixmap(dpy, r);
	}
	r = PCreateRenderPixmap(
		dpy, win, ParentRelative, None, None, Pdepth, 100, fra->tint,
		fra->tint_percent,
		True, win,
		gc, None, None, gx, gy, gw, gh, gx, gy, gw, gh,
		False, &dummy, &dummy, &dummy, &dp);

 bail:
	if (old_backing_store >= 0)
	{
		FSetBackingStore(dpy, win, old_backing_store);
	}
	return r;
}

void PGraphicsTintRectangle(
	Display *dpy, Window win, Pixel tint, int tint_percent,
	Drawable dest, Bool dest_is_a_window, GC gc, GC mono_gc, GC alpha_gc,
	int dest_x, int dest_y, int dest_w, int dest_h)
{
	Pixmap p;
	FvwmRenderAttributes fra;

#if 0
	/* this does not work. why? */
	if (FRenderTintRectangle(
		dpy, win, None, tint, tint_percent, dest,
		dest_x, dest_y, dest_w, dest_h))
	{
		return;
	}
#else

	if (FRenderRender(
		dpy, win, ParentRelative, None, None, Pdepth, 100,
		tint, tint_percent, win, gc, None,
		dest_x, dest_y, dest_w, dest_h,
		dest_x, dest_y, dest_w, dest_h, False))
	{

		return;
	}
#endif

	if (dest_is_a_window)
	{
		fra.tint = tint;
		fra.tint_percent = tint_percent;
		fra.mask = FRAM_DEST_IS_A_WINDOW | FRAM_HAVE_TINT;
		p = PGraphicsCreateTransparency(
			dpy, dest, &fra, gc, dest_x, dest_y, dest_w, dest_h,
			False);
		if (p)
		{
			XCopyArea(
				dpy, p, dest, gc, 0, 0, dest_w, dest_h,
				dest_x, dest_y);
			XFreePixmap(dpy, p);
		}
	}
}

#if 0 /* humm... maybe useful one day with menus */
Pixmap PGraphicsCreateTranslucent(
	Display *dpy, Window win, FvwmRenderAttributes *fra, GC gc,
	int x, int y, int width, int height)
{
	Pixmap r = None;
	int gx = x, gy = y, gh = height, gw = width;
	FvwmRenderAttributes t_fra;
	Pixmap root_pix = None;
	Pixmap dp = None;
	int dummy;

	t_fra.added_alpha_percent = 100;
	t_fra.tint_percent = 0;
	t_fra.tint = 0;
	t_fra.mask = 0;

	if (fra)
	{
		if (fra->mask & FRAM_HAVE_TINT)
		{
			t_fra.tint_percent = fra->tint_percent;
			t_fra.tint = fra->tint;
			t_fra.mask = FRAM_HAVE_TINT;
		}
	}

	if (x >= DisplayWidth(dpy, DefaultScreen(dpy)))
	{
		goto bail;
	}
	if (y >= DisplayHeight(dpy, DefaultScreen(dpy)))
	{
		goto bail;
	}
	if (x < 0)
	{
		gx = 0;
		gw = width + x;
		if (gw <= 0)
		{
			goto bail;
		}
	}
	if (y < 0)
	{
		gy = 0;
		gh = gh+y;
		if (gh <= 0)
		{
			goto bail;
		}
	}
	if (gx + gw > DisplayWidth(dpy, DefaultScreen(dpy)))
	{
		gw = DisplayWidth(dpy, DefaultScreen(dpy)) - gx;
	}
	if (gy + gh > DisplayHeight(dpy, DefaultScreen(dpy)))
	{
		gh = DisplayHeight(dpy, DefaultScreen(dpy)) - gy;
	}
	{
		/* make a screen shoot */
		GC my_gc;
		unsigned long valuemask = GCSubwindowMode;
		XGCValues values;

		values.subwindow_mode = IncludeInferiors;
		root_pix = XCreatePixmap(dpy, win, gw, gh, Pdepth);
		my_gc = fvwmlib_XCreateGC(dpy, win, 0, NULL);
		XChangeGC(dpy, my_gc, valuemask, &values);
		MyXGrabServer(dpy);
		XCopyArea(
			dpy, DefaultRootWindow(dpy), root_pix, my_gc,
			gx, gy, gw, gh, 0, 0);
		MyXUngrabServer(dpy);
		XFreeGC(dpy,my_gc);
	}
	if (XRenderSupport && FRenderGetExtensionSupported())
	{
		r = XCreatePixmap(dpy, win, gw, gh, Pdepth);
		if (FRenderRender(
			dpy, win, root_pix, None, None, Pdepth,
			t_fra.added_alpha_percent, t_fra.tint,
			t_fra.tint_percent, r, gc, None,
			0, 0, gw, gh, 0, 0, gw, gh, False))
		{
			goto bail;
		}
		XFreePixmap(dpy, r);
		r = None;
	}
	r = PCreateRenderPixmap(
		dpy, win, root_pix, None, None, Pdepth, 100,
		fra->tint, fra->tint_percent, True, win,
		gc, None, None, 0, 0, gw, gh, gx, gy, gw, gh,
		False, &dummy, &dummy, &dummy, &dp);

 bail:
	if (root_pix)
	{
		XFreePixmap(dpy, root_pix);
	}
	if (dp)
	{
		XFreePixmap(dpy, dp);
	}
	return r;
}
#endif


/* never tested and used ! */
Pixmap PGraphicsCreateDitherPixmap(
	Display *dpy, Window win, Drawable src, Pixmap mask, int depth, GC gc,
	int in_width, int in_height, int out_width, int out_height)
{
	return PCreateDitherPixmap(
		dpy, win, src, mask, depth, gc,
		in_width, in_height, out_width, out_height);
}
