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
#include "PictureGraphics.h"
#include "FRenderInit.h"
#include "FRender.h"
#include "FRenderInterface.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

static FRenderPictFormat *PFrenderVisualFormat = NULL;
static FRenderPictFormat *PFrenderAlphaFormat = NULL;
static FRenderPictFormat *PFrenderMaskFormat = NULL;
static FRenderPictFormat *PFrenderDirectFormat = NULL;
static FRenderPictFormat *PFrenderAbsoluteFormat = NULL;
Bool FRenderVisualInitialized = False;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

static
void FRenderVisualInit(Display *dpy)
{
	FRenderPictFormat pf;

	if (!XRenderSupport || !FRenderGetExtensionSupported())
	{
		return;
	}

	PFrenderVisualFormat = FRenderFindVisualFormat (dpy, Pvisual);
	if (!PFrenderVisualFormat)
	{
		fprintf(stderr,"[fvwmlibs][FRenderInit] -- ERROR: "
			"fail to create XRender Visual Format\n");
		return;
	}
	pf.depth = 8;
	pf.type = FRenderPictTypeDirect;
	pf.direct.alpha = 0;
	pf.direct.alphaMask = 0xff;
	PFrenderAlphaFormat = FRenderFindFormat(dpy,
						FRenderPictFormatType|
						FRenderPictFormatDepth|
						FRenderPictFormatAlpha|
						FRenderPictFormatAlphaMask,
						&pf, 0);
	if (!PFrenderAlphaFormat)
	{
		fprintf(stderr,"[fvwmlibs][FRenderInit] -- ERROR: "
			"fail to create XRender Visual Format\n");
		return;
	}
	pf.depth = 1;
	pf.type = FRenderPictTypeDirect;
	pf.direct.alpha = 0;
	pf.direct.alphaMask = 1;
	PFrenderMaskFormat = FRenderFindFormat(dpy,
						FRenderPictFormatType|
						FRenderPictFormatDepth|
						FRenderPictFormatAlpha|
						FRenderPictFormatAlphaMask,
						&pf, 0);
	if (!PFrenderMaskFormat)
	{
		fprintf(stderr,"[fvwmlibs][FRenderInit] -- ERROR: "
			"fail to create XRender Mask Format\n");
		return;
	}
	pf.depth = 24;
	pf.type = FRenderPictTypeDirect;
	pf.direct.alpha = 0;
	pf.direct.alphaMask = 0;
	pf.direct.red = 16;
	pf.direct.redMask = 0xff;
	pf.direct.green = 8;
	pf.direct.greenMask = 0xff;
	pf.direct.blue = 0;
	pf.direct.blueMask = 0xff;
	PFrenderDirectFormat = FRenderFindFormat(dpy,
						 FRenderPictFormatType|
						 FRenderPictFormatDepth|
						 FRenderPictFormatRed|
						 FRenderPictFormatRedMask|
						 FRenderPictFormatGreen|
						 FRenderPictFormatGreenMask|
						 FRenderPictFormatBlue|
						 FRenderPictFormatBlueMask|
						 FRenderPictFormatAlpha|
						 FRenderPictFormatAlphaMask,
						 &pf, 0);
	if (!PFrenderDirectFormat)
	{
		fprintf(stderr,"[fvwmlibs][FRenderInit] -- ERROR: "
			"fail to create XRender Direct Format\n");
		return;
	}
	pf.depth = 32;
	pf.type = FRenderPictTypeDirect;
	pf.direct.alpha = 24;
	pf.direct.alphaMask = 0xff;
	pf.direct.red = 16;
	pf.direct.redMask = 0xff;
	pf.direct.green = 8;
	pf.direct.greenMask = 0xff;
	pf.direct.blue = 0;
	pf.direct.blueMask = 0xff;
	PFrenderAbsoluteFormat = FRenderFindFormat(dpy,
						 FRenderPictFormatType|
						 FRenderPictFormatDepth|
						 FRenderPictFormatRed|
						 FRenderPictFormatRedMask|
						 FRenderPictFormatGreen|
						 FRenderPictFormatGreenMask|
						 FRenderPictFormatBlue|
						 FRenderPictFormatBlueMask|
						 FRenderPictFormatAlpha|
						 FRenderPictFormatAlphaMask,
						 &pf, 0);
	if (!PFrenderAbsoluteFormat)
	{
		fprintf(stderr,"[fvwmlibs][FRenderInit] -- ERROR: "
			"fail to create XRender Absolute Format\n");
		return;
	}
}

/* ---------------------------- interface functions ------------------------- */

int FRenderRender(
	Display *dpy, Window win, Pixmap pixmap, Pixmap mask, Pixmap alpha,
	int depth, int added_alpha_percent, Pixel tint, int tint_percent,
	Drawable d, GC gc, GC alpha_gc,
	int src_x, int src_y, int src_w, int src_h,
	int dest_x, int dest_y, int dest_w, int dest_h,
	Bool do_repeat)
{
	static Pixmap shade_pixmap = None;
	static FRenderPicture shade_picture = None;
	static int saved_added_alpha_percent = 0;
	static Pixel saved_tint = 0;
	static Pixmap tint_pixmap = None;
	static FRenderPicture tint_picture = None;
	static int saved_tint_percent = 0;
	FRenderColor frc;
	Pixmap pixmap_copy = None, alpha_copy = None;
	FRenderPicture alpha_picture = None, mask_picture = None;
	FRenderPicture dest_picture = None, src_picture = None;
	FRenderPictureAttributes  pa, e_pa;
	unsigned long pam = 0;
	int alpha_x = src_x, alpha_y = src_y, rv = 0;
	Bool force_update = False, free_gc = False, free_alpha_gc = False;

	if (!XRenderSupport || !FRenderGetExtensionSupported())
	{
		return 0;
	}

	if (!FRenderVisualInitialized)
	{
		FRenderVisualInitialized = True;
		FRenderVisualInit(dpy);
	}

	if (!PFrenderVisualFormat || !PFrenderAlphaFormat ||
	    !PFrenderAbsoluteFormat || !PFrenderMaskFormat)
	{
		return 0;
	}

	/* it is a bitmap ? */
	if (Pdepth != depth)
	{
		pixmap_copy = XCreatePixmap(dpy, win, src_w, src_h, Pdepth);
		if (gc == None)
		{
			XGCValues gcv;

			gc = fvwmlib_XCreateGC(dpy, d, 0, NULL);
			gcv.foreground = WhitePixel(dpy, DefaultScreen(dpy));
			gcv.background = BlackPixel(dpy, DefaultScreen(dpy));
			XChangeGC(dpy, gc, GCBackground|GCForeground, &gcv);
			free_gc = True;
		}
		if (pixmap_copy && gc)
		{
			XCopyPlane(
				dpy, pixmap, pixmap_copy, gc,
				src_x, src_y, src_w, src_h,
				0, 0, 1);
		}
		src_x = src_y = 0;
	}

	pam = FRenderCPRepeat;
	if (do_repeat)
	{
		pa.repeat = True;
	}
	else
	{
		pa.repeat = False;
	}

	if (tint_percent <= 0)
	{
		src_picture = FRenderCreatePicture(
			dpy,
			(pixmap_copy != None)? pixmap_copy:pixmap,
			PFrenderVisualFormat, pam, &pa);
	}
	else
	{
		if (!tint_pixmap || !tint_picture)
		{
			if (win == None)
			{
				win = RootWindow(dpy, DefaultScreen(dpy));
			}
			e_pa.repeat = True;
			if (!tint_pixmap)
			{
				tint_pixmap = XCreatePixmap(dpy, win, 1, 1, 32);
			}
			if (tint_pixmap)
			{
				tint_picture = FRenderCreatePicture(
					dpy, tint_pixmap,
					PFrenderAbsoluteFormat,
					FRenderCPRepeat, &e_pa);
			}
			force_update = True;
		}
		if (tint_picture && 
		    (tint != saved_tint || tint_percent != saved_tint_percent
		     || force_update))
		{
			XColor color;
			float alpha_factor = (float)tint_percent/100;

			force_update = False;
			color.pixel = tint;
			XQueryColor(dpy, Pcmap, &color);
			frc.red = color.red * alpha_factor;
			frc.green = color.green * alpha_factor;
			frc.blue = color.blue * alpha_factor;
			frc.alpha = 0xffff * alpha_factor;
			FRenderFillRectangle(
				dpy, FRenderPictOpSrc, tint_picture, &frc,
				0, 0, 1, 1);
			saved_tint = tint;
		}
		if (win == None)
		{
			win = RootWindow(dpy, DefaultScreen(dpy));
		}
		if (pixmap_copy == None)
		{
			if (gc == None)
			{
				gc = fvwmlib_XCreateGC(dpy, pixmap, 0, NULL);
				free_gc = True;
			}
			pixmap_copy = XCreatePixmap(
				dpy, win, src_w, src_h, Pdepth);
			if (pixmap_copy && gc)
			{
				XCopyArea(
					dpy, pixmap, pixmap_copy, gc,
					src_x, src_y, src_w, src_h, 0, 0);
			}
		}
		if (pixmap_copy)
		{
			src_picture = FRenderCreatePicture(
				dpy, pixmap_copy, PFrenderVisualFormat,
				pam, &pa);
		}
		if (tint_picture && src_picture)
		{
			FRenderComposite(
				dpy, FRenderPictOpOver, tint_picture, None,
				src_picture, 0, 0, 0, 0, 0, 0, src_w, src_h);
		}
		src_x = src_y = 0;
	}

	if (free_gc && gc)
	{
		XFreeGC(dpy, gc);
	}

	if (!src_picture)
	{
		if (pixmap_copy)
		{
			XFreePixmap(dpy, pixmap_copy);
		}
		return 0;
	}

	if (added_alpha_percent >= 100)
	{
		if (alpha != None)
		{
			alpha_picture = FRenderCreatePicture(
				dpy, alpha, PFrenderAlphaFormat, pam, &pa);
		}
		else if (mask != None)
		{
			alpha_picture = FRenderCreatePicture(
				dpy, mask, PFrenderMaskFormat, pam, &pa);
		}
	}
	else
	{
		if (win == None && (alpha != None || mask != None))
		{
			win = RootWindow(dpy, DefaultScreen(dpy));
		}
		if (alpha != None)
		{
			alpha_copy = XCreatePixmap(dpy, win, src_w, src_h, 8);
			if (!alpha_gc)
			{
				alpha_gc = fvwmlib_XCreateGC(
					dpy, alpha, 0, NULL);
				free_alpha_gc = True;
			}
			if (alpha_copy && alpha_gc)
			{
				XCopyArea(dpy, alpha, alpha_copy, alpha_gc,
					  src_x, src_y, src_w, src_h, 0, 0);
				alpha_picture = FRenderCreatePicture(
					dpy, alpha_copy, PFrenderAlphaFormat,
					pam, &pa);
			}
			if (alpha_gc && free_alpha_gc)
			{
				XFreeGC(dpy, alpha_gc);
			}
		}
		else if (mask != None)
		{
			alpha_copy = XCreatePixmap(dpy, win, src_w, src_h, 8);
			if (alpha_copy)
			{
				alpha_picture = FRenderCreatePicture(
					dpy, alpha_copy, PFrenderAlphaFormat,
					pam, &pa);
			}
			if (alpha_picture)
			{
				frc.red = frc.green = frc.blue = frc.alpha = 0;
				FRenderFillRectangle(
					dpy, FRenderPictOpSrc, alpha_picture,
					&frc, 0, 0, src_w, src_h);
			}
			mask_picture = FRenderCreatePicture(
				dpy, mask, PFrenderMaskFormat, pam, &pa);
		}

		if (!shade_pixmap || !shade_picture)
		{
			pa.repeat = True;

			if (win == None)
			{
				win = RootWindow(dpy, DefaultScreen(dpy));
			}
			if (!shade_pixmap)
			{
				shade_pixmap = XCreatePixmap(dpy, win, 1, 1, 8);
			}
			e_pa.repeat = True;
			if (shade_pixmap)
			{
				shade_picture = FRenderCreatePicture(
					dpy, shade_pixmap, PFrenderAlphaFormat,
					FRenderCPRepeat, &e_pa);
			}
			force_update = True;
		}
		if (shade_picture && 
		    (added_alpha_percent != saved_added_alpha_percent
		     || force_update))
		{
			frc.red = frc.green = frc.blue = 0;
			frc.alpha = 0xffff * (added_alpha_percent)/100;
			FRenderFillRectangle(
				dpy, FRenderPictOpSrc, shade_picture, &frc,
				0, 0, 1, 1);
			saved_added_alpha_percent = added_alpha_percent;
		}

		if (alpha != None && alpha_picture && shade_picture)
		{
			FRenderComposite(
				dpy, PictOpAtopReverse, shade_picture,
				alpha_picture, alpha_picture,
				0, 0, 0, 0, 0, 0, src_w, src_h);
			alpha_x = alpha_y = 0;
		}
		else if (mask != None && alpha_picture && shade_picture)
		{
			FRenderComposite(
				dpy, PictOpAtopReverse, shade_picture,
				mask_picture, alpha_picture,
				0, 0, 0, 0, 0, 0, src_w, src_h);
			alpha_x = alpha_y = 0;
		}
		else
		{
			alpha_picture = shade_picture;
		}
	}

	dest_picture = FRenderCreatePicture(dpy, d, PFrenderVisualFormat, 0, 0);

	if (dest_picture)
	{
		rv = 1;
		FRenderComposite(dpy,
				 FRenderPictOpOver,
				 src_picture,
				 alpha_picture,
				 dest_picture,
				 src_x, src_y, alpha_x, alpha_y,
				 dest_x, dest_y, dest_w, dest_h);
	}

	if (dest_picture)
	{
		FRenderFreePicture(dpy, dest_picture);
	}
	if (src_picture)
	{
		FRenderFreePicture(dpy, src_picture);
	}
	if (alpha_picture && alpha_picture != shade_picture)
	{
		FRenderFreePicture(dpy, alpha_picture);
	}
	if (mask_picture)
	{
		FRenderFreePicture(dpy, mask_picture);
	}
	if (alpha_copy)
	{
		XFreePixmap(dpy, alpha_copy);
	}
	if (pixmap_copy)
	{
		XFreePixmap(dpy, pixmap_copy);
	}
	return rv;
}
