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
#include "Colorset.h"
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

static Bool PGrabImageError = True;

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
	gcv.clip_x_origin = dest_x - src_x;
	gcv.clip_y_origin = dest_y - src_y;
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
	gcv.clip_mask = tile_mask;
	gcv.fill_style = FillTiled;
	gcv.tile = pixmap;
	gcv.ts_x_origin = dest_x;
	gcv.ts_y_origin = dest_y;
	gcv.clip_x_origin = dest_x;
	gcv.clip_y_origin = dest_y;;
	gcm = GCFillStyle | GCClipMask | GCTile | GCTileStipXOrigin |
		GCTileStipYOrigin | GCClipXOrigin | GCClipYOrigin;
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
Pixmap PCreateRenderPixmap(
	Display *dpy, Window win, Pixmap pixmap, Pixmap mask, Pixmap alpha,
	int depth, int added_alpha_percent, Pixel tint, int tint_percent,
	Bool d_is_a_window, Drawable d, GC gc, GC mono_gc, GC alpha_gc,
	int src_x, int src_y, int src_w, int src_h,
	int dest_x, int dest_y, int dest_w, int dest_h, Bool do_repeat,
	int *new_w, int *new_h, Bool *new_do_repeat,
	Pixmap *new_mask)
{
	XImage *pixmap_im = NULL;
	XImage *mask_im = NULL;
	XImage *alpha_im = NULL;
	XImage *dest_im = NULL;
	XImage *new_mask_im = NULL;
	XImage *out_im = NULL;
	Pixmap pixmap_copy = None;
	Pixmap src_pix = None;
	Pixmap out_pix = None;
	unsigned short *am = NULL;
	XColor *colors = NULL, *dest_colors = NULL;
	XColor tint_color, c;
	int w ,h, n_src_w, n_src_h;
	int j, i, j1, i1, m = 0, k = 0, l = 0;
	Bool do_free_gc = False;
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
			XGCValues gcv;

			gc = fvwmlib_XCreateGC(dpy, d, 0, NULL);
			gcv.foreground = WhitePixel(dpy, DefaultScreen(dpy));
			gcv.background = BlackPixel(dpy, DefaultScreen(dpy));
			XChangeGC(dpy, gc, GCBackground|GCForeground, &gcv);
			do_free_gc = True;
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

	if (!(pixmap_im =
	      XGetImage(dpy, src_pix, src_x, src_y, src_w, src_h, AllPlanes,
			ZPixmap)))
	{
		error = True;
		goto bail;
	}
	if (mask != None)
	{
		mask_im = XGetImage(
			dpy, mask, src_x, src_y, src_w, src_h, AllPlanes,
			ZPixmap);
		if (src_x != 0 || src_y != 0)
			make_new_mask = True;
	}
	if (alpha != None)
	{
		alpha_im = XGetImage(
			dpy, alpha, src_x, src_y, src_w, src_h, AllPlanes,
			ZPixmap);
	}

	if (alpha != None || added_alpha_percent < 100)
	{
		Bool try_to_grab = True;
		XWindowAttributes   xwa;
		XErrorHandler saved_eh = NULL;

		PGrabImageError = 0;
		if (d_is_a_window)
		{
			XGrabServer(dpy);
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
			dest_im = XGetImage(
				dpy, d, dest_x, dest_y, dest_w, dest_h,
				AllPlanes, ZPixmap);
			if (PGrabImageError)
			{
				dest_im = NULL;
			}
			if (d_is_a_window)
			{
				XSetErrorHandler((XErrorHandler) saved_eh);	
			}
		}
	
		if (d_is_a_window)
		{
			XUngrabServer(dpy);
		}
	}

	if (dest_im && do_repeat && (dest_w > src_w || dest_h > src_h))
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
	out_im = XCreateImage(
		dpy, Pvisual, Pdepth, ZPixmap, 0, 0, w, h,
		Pdepth > 16 ? 32 : (Pdepth > 8 ? 16 : 8), 0);
	if (gc == None)
	{
		gc = fvwmlib_XCreateGC(dpy, win, 0, NULL);
		do_free_gc = True;
	}

	if (!out_pix || !out_im || !gc)
	{
		error = True;
		goto bail;
	}

	colors = (XColor *)safemalloc(n_src_w * n_src_h * sizeof(XColor));
	if (dest_im)
	{
		dest_colors =
			(XColor *)safemalloc(w * h * sizeof(XColor));
	}
	am = (unsigned short *)safemalloc(
		n_src_w * n_src_h * sizeof(unsigned short));
	out_im->data = safemalloc(out_im->bytes_per_line * h);

	if (tint_percent > 0)
	{
		tint_color.pixel = tint;
		XQueryColor(dpy, Pcmap, &tint_color);
	}

	for (j = 0; j < n_src_h; j++)
	{
		for (i = 0; i < n_src_w; i++, m++)
		{
			if (mask_im != None && (XGetPixel(mask_im, i, j) == 0))
			{
				am[m] = 0;
			}
			else if (alpha_im != None)
			{
				am[m] = XGetPixel(alpha_im, i, j);
				if (am[m] == 0 && !dest_im)
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
				if (!dest_im)
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
						XGetPixel(dest_im, i, j);
				}
				if (am[m] > 0)
				{
					colors[k++].pixel =
						XGetPixel(pixmap_im, i, j);
				}
			}
		}
	}

	for (i = 0; i < k; i += 256)
		XQueryColors(dpy, Pcmap, &colors[i], min(k - i, 256));

	if (do_repeat && dest_im && (n_src_h < h || n_src_w < w))
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
									dest_im,
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
			 new_mask_im = XCreateImage(
				 dpy, Pvisual, 1, ZPixmap, 0, 0, w, h,
				 Pdepth > 16 ? 32 : (Pdepth > 8 ? 16 : 8), 0);
			 if (mono_gc == None)
			 {
				 mono_gc = fvwmlib_XCreateGC(
					 dpy, *new_mask, 0, NULL);
				 do_free_mono_gc = True;
			 }
			 new_mask_im->data = safemalloc(
				 new_mask_im->bytes_per_line * h);
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
					XPutPixel(
						new_mask_im, i, j,
						WhitePixel(
							dpy,
							DefaultScreen(dpy)));
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
				if (am[m] < 255 && dest_im)
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
				if (dest_im)
				{
					c.pixel = XGetPixel(dest_im, i, j);
				}
				else
				{
					c.pixel = XGetPixel(pixmap_im, i, j);
				}
				if (*new_mask)
				{
					XPutPixel(new_mask_im, i, j, 0);
				}
			}
			XPutPixel(out_im, i, j, c.pixel);
		}
	}

	/* tile: editor ligne width limit  107 !!*/
	if (do_repeat && dest_im && (n_src_h < h || n_src_w < w))
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
										new_mask_im, i+i1, j+j1,
										WhitePixel(
											dpy,
											DefaultScreen(
												dpy)));
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
								c.pixel = XGetPixel(dest_im, i+i1, j+j1);
								if (*new_mask)
								{
									XPutPixel(
										new_mask_im, i+i1, j+j1, 0);
								}
							}
							XPutPixel(out_im, i+i1, j+j1, c.pixel);
						}
					}
				}
			}
		}
	}

	XPutImage(dpy, out_pix, gc, out_im, 0, 0, 0, 0, w, h);
	if (*new_mask && mono_gc)
	{
		XPutImage(dpy, *new_mask, mono_gc, new_mask_im,
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
	if (pixmap_im)
	{
		XDestroyImage(pixmap_im);
	}
	if (mask_im)
	{
		XDestroyImage(mask_im);
	}
	if (alpha_im)
	{
		XDestroyImage(alpha_im);
	}
	if (dest_im)
	{
		XDestroyImage(dest_im);
	}
	if (do_free_gc && gc)
	{
		XFreeGC(dpy, gc);
	}
	if (new_mask_im)
	{
		XDestroyImage(new_mask_im);
	}
	if (do_free_mono_gc && mono_gc)
	{
		XFreeGC(dpy, mono_gc);
	}
	if (out_im)
	{
		XDestroyImage(out_im);
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
	XImage *src_im;
	XImage *mask_im = NULL;
	XImage *out_im;
	GC my_gc;
	Pixmap out_pix = None;
	unsigned char *cm;
	XColor *colors;
	XColor c;
	int j, i, m = 0, k = 0, x = 0, y = 0;

	if (depth != Pdepth)
		return None;

	if (!(src_im =
	      XGetImage(dpy, src, 0, 0,
			in_width, in_height, AllPlanes, ZPixmap)))
	{
		return None;
	}
	if (mask != None)
	{
		mask_im = XGetImage(
			dpy, mask, 0, 0, in_width, in_height, AllPlanes,
			ZPixmap);
	}
	out_pix = XCreatePixmap(dpy, win, out_width, out_height, Pdepth);
	out_im = XCreateImage(
		dpy, Pvisual, Pdepth, ZPixmap, 0, 0, out_width, out_height,
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

	colors = (XColor *)safemalloc(out_width * out_height * sizeof(XColor));
	cm = (unsigned char *)safemalloc(out_width * out_height * sizeof(char));
	out_im->data = safemalloc(out_im->bytes_per_line * out_height);

	x = y = 0;
	for (j = 0; j < out_height; j++,y++)
	{
		if (y == in_height)
			y = 0;
		for (i = 0; i < out_width; i++,x++)
		{
			if (x == in_width)
				x = 0;
			if (mask_im != NULL &&
			    (XGetPixel(mask_im, x, y) == 0))
			{
				cm[m++] = 0;
			}
			else
			{
				cm[m++] = 255;
				colors[k++].pixel = XGetPixel(src_im, x, y);
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
	XPutImage(
		dpy, out_pix, my_gc, out_im, 0, 0, 0, 0, out_width, out_height);
	if (gc == None)
	{
		XFreeGC(dpy, my_gc);
	}
	XDestroyImage(out_im);

	return out_pix;
}

/* ---------------------------- interface functions ------------------------- */
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

	if (fra)
	{
		t_fra.mask = fra->mask;
		if (fra->mask & FRAM_HAVE_ICON_CSET)
		{
			t_fra.added_alpha_percent = fra->colorset->icon_alpha;
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
	/* TMP: disabling Xrender rendering for testing */
	if (0 && FRenderRender(
		dpy, win, pixmap, mask, alpha, depth,
		t_fra.added_alpha_percent, t_fra.tint, t_fra.tint_percent,
		d, gc, alpha_gc, src_x, src_y, dest_w, dest_h,
		dest_x, dest_y, dest_w, dest_h, do_repeat))
	{
		return;
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
			dpy, src->alpha, src->width, src->height, 8,
			dest_width, dest_height, alpha_gc);
	}

	q = (FvwmPicture*)safemalloc(sizeof(FvwmPicture));
	memset(q, 0, sizeof(FvwmPicture));
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

/* never tested and used ! */
Pixmap PGraphicsCreateDitherPixmap(
	Display *dpy, Window win, Drawable src, Pixmap mask, int depth, GC gc,
	int in_width, int in_height, int out_width, int out_height)
{
	return PCreateDitherPixmap(
		dpy, win, src, mask, depth, gc,
		in_width, in_height, out_width, out_height);
}
