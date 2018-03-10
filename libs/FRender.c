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
#include "Graphics.h"
#include "PictureGraphics.h"
#include "FRenderInit.h"
#include "FRender.h"
#include "FRenderInterface.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static FRenderPictFormat *PFrenderVisualFormat = NULL;
static FRenderPictFormat *PFrenderAlphaFormat = NULL;
static FRenderPictFormat *PFrenderMaskFormat = NULL;
static FRenderPictFormat *PFrenderDirectFormat = NULL;
static FRenderPictFormat *PFrenderAbsoluteFormat = NULL;
Bool FRenderVisualInitialized = False;

/* #define USE_ABSOLUTE_FORMAT 1*/

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

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
	SUPPRESS_UNUSED_VAR_WARNING(pf);
	PFrenderAlphaFormat = FRenderFindFormat(
		dpy, FRenderPictFormatType| FRenderPictFormatDepth|
		FRenderPictFormatAlpha| FRenderPictFormatAlphaMask, &pf, 0);
	if (!PFrenderAlphaFormat)
	{
		fprintf(stderr,"[fvwmlibs][FRenderInit] -- ERROR: "
			"fail to create XRender Alpha Format\n");
		return;
	}
	pf.depth = 1;
	pf.type = FRenderPictTypeDirect;
	pf.direct.alpha = 0;
	pf.direct.alphaMask = 1;
	PFrenderMaskFormat = FRenderFindFormat(
		dpy, FRenderPictFormatType| FRenderPictFormatDepth|
		FRenderPictFormatAlpha| FRenderPictFormatAlphaMask, &pf, 0);
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
	PFrenderDirectFormat = FRenderFindFormat(
		dpy, FRenderPictFormatType| FRenderPictFormatDepth|
		FRenderPictFormatRed| FRenderPictFormatRedMask|
		FRenderPictFormatGreen| FRenderPictFormatGreenMask|
		FRenderPictFormatBlue| FRenderPictFormatBlueMask|
		FRenderPictFormatAlpha| FRenderPictFormatAlphaMask, &pf, 0);
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
	PFrenderAbsoluteFormat = FRenderFindFormat(
		dpy, FRenderPictFormatType| FRenderPictFormatDepth|
		FRenderPictFormatRed| FRenderPictFormatRedMask|
		FRenderPictFormatGreen| FRenderPictFormatGreenMask|
		FRenderPictFormatBlue| FRenderPictFormatBlueMask|
		FRenderPictFormatAlpha| FRenderPictFormatAlphaMask, &pf, 0);
	if (!PFrenderAbsoluteFormat)
	{
		fprintf(stderr,"[fvwmlibs][FRenderInit] -- ERROR: "
			"fail to create XRender Absolute Format\n");
		return;
	}
}

static
Bool FRenderCompositeAndCheck(
	Display *dpy, int op, FRenderPicture src, FRenderPicture alpha,
	FRenderPicture dest, int x, int y, int a_x, int a_y,
	int d_x, int d_y, int d_w, int d_h)
{
	FRenderComposite(
		dpy, op, src, alpha, dest, x, y, a_x, a_y, d_x, d_y, d_w, d_h);

	return True;

}

static
Bool FRenderCreateShadePicture(
	Display *dpy, Window win, int alpha_percent)
{
	static Pixmap shade_pixmap = None;
	static FRenderPicture shade_picture = None;
	static int saved_alpha_percent = 0;
	Bool force_update = False;
	FRenderColor frc;

	if (!XRenderSupport || !FRenderGetExtensionSupported())
	{
		return 0;
	}

	/* FRender Visuals should be already initialized */

	if (!shade_pixmap || !shade_picture)
	{
		FRenderPictureAttributes  pa;

		if (!shade_pixmap)
		{
			shade_pixmap = XCreatePixmap(dpy, win, 1, 1, 8);
		}
		pa.repeat = True;
		if (shade_pixmap)
		{
			SUPPRESS_UNUSED_VAR_WARNING(pa);
			shade_picture = FRenderCreatePicture(
				dpy, shade_pixmap, PFrenderAlphaFormat,
				FRenderCPRepeat, &pa);
		}
		force_update = True;
	}
	if (shade_picture &&
	    (alpha_percent != saved_alpha_percent || force_update))
	{
		frc.red = frc.green = frc.blue = 0;
		frc.alpha = 0xffff * (alpha_percent)/100;
		FRenderFillRectangle(
			dpy, FRenderPictOpSrc, shade_picture, &frc,
			0, 0, 1, 1);
		saved_alpha_percent = alpha_percent;
	}

	return shade_picture;
}

static
Bool FRenderTintPicture(
	Display *dpy, Window win, Pixel tint, int tint_percent,
	FRenderPicture dest_picture,
	int dest_x, int dest_y, int dest_w, int dest_h)
{
	static Pixel saved_tint = 0;
	static int saved_tint_percent = 0;
	static Pixmap tint_pixmap = None;
	static FRenderPicture tint_picture = None;
	FRenderPicture shade_picture = None;
	Bool force_update = False;
	FRenderPictureAttributes  pa;
	int rv = 0;

	if (!XRenderSupport)
	{
		return 0;
	}

	if (!tint_pixmap || !tint_picture)
	{
		pa.repeat = True;
		if (!tint_pixmap)
		{
			tint_pixmap = XCreatePixmap(dpy, win, 1, 1, 32);
		}
		if (tint_pixmap)
		{
			tint_picture = FRenderCreatePicture(
				dpy, tint_pixmap,
				PFrenderAbsoluteFormat,
				FRenderCPRepeat, &pa);
			if (!tint_picture)
			{
				goto bail;
			}
		}
		else
		{
			goto bail;
		}
		force_update = True;
	}
	if (tint_picture &&
	    (tint != saved_tint || tint_percent != saved_tint_percent ||
	     force_update))
	{
		XColor color;
		float alpha_factor = (float)tint_percent/100;
		FRenderColor frc_tint;

		force_update = False;
		color.pixel = tint;
		XQueryColor(dpy, Pcmap, &color);
		frc_tint.red = color.red * alpha_factor;
		frc_tint.green = color.green * alpha_factor;
		frc_tint.blue = color.blue * alpha_factor;
		frc_tint.alpha = 0xffff * alpha_factor;
		SUPPRESS_UNUSED_VAR_WARNING(frc_tint);
		FRenderFillRectangle(
			dpy, FRenderPictOpSrc, tint_picture, &frc_tint,
			0, 0, 1, 1);
		saved_tint = tint;
		saved_tint_percent = tint_percent;
	}
	if (!shade_picture)
	{
		shade_picture = FRenderCreateShadePicture(dpy, win, 100);
	}

	rv = FRenderCompositeAndCheck(
		dpy, FRenderPictOpOver, tint_picture, shade_picture,
		dest_picture, 0, 0, 0, 0, dest_x, dest_y, dest_w, dest_h);
	SUPPRESS_UNUSED_VAR_WARNING(pa);

 bail:
	return rv;
}

/* ---------------------------- interface functions ------------------------ */


Bool FRenderTintRectangle(
	Display *dpy, Window win, Pixmap mask, Pixel tint, int tint_percent,
	Drawable d, int dest_x, int dest_y, int dest_w, int dest_h)
{
	FRenderPicture dest_picture = None;
	FRenderPictureAttributes  pa;
	unsigned int val;
	Bool rv = True;

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

	pa.clip_mask = mask;
	val = FRenderCPClipMask;
	SUPPRESS_UNUSED_VAR_WARNING(val);
	SUPPRESS_UNUSED_VAR_WARNING(pa);
	if (!(dest_picture = FRenderCreatePicture(
		dpy, d, PFrenderVisualFormat, val, &pa)))
	{
		return 0;
	}

	rv = FRenderTintPicture(
		dpy, win, tint_percent, tint, dest_picture, dest_x, dest_y,
		dest_w, dest_h);

	FRenderFreePicture(dpy, dest_picture);
	return rv;
}

int FRenderRender(
	Display *dpy, Window win, Pixmap pixmap, Pixmap mask, Pixmap alpha,
	int depth, int added_alpha_percent, Pixel tint, int tint_percent,
	Drawable d, GC gc, GC alpha_gc,
	int src_x, int src_y, int src_w, int src_h,
	int dest_x, int dest_y, int dest_w, int dest_h,
	Bool do_repeat)
{
	FRenderColor frc;
	Pixmap pixmap_copy = None;
	Pixmap alpha_copy = None;
	FRenderPicture shade_picture = None;
	FRenderPicture alpha_picture = None;
	FRenderPicture mask_picture = None;
	FRenderPicture src_picture = None;
	FRenderPicture dest_picture = None;
	FRenderPicture root_picture = None;
	FRenderPictureAttributes  pa;
	unsigned long pam = 0;
	int alpha_x = src_x;
	int alpha_y = src_y;
	Bool rv = False;
	Bool free_alpha_gc = False;

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
	if (Pdepth != depth && pixmap)
	{
		pixmap_copy = PictureBitmapToPixmap(
			dpy, win, pixmap, Pdepth, gc, src_x, src_y, src_w,
			src_h);
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

	/*
	 * build the src_picture
	 */
	if (pixmap == ParentRelative)
	{
		/* need backing store and good preparation of the win */
		if (gc == None)
		{
			gc = PictureDefaultGC(dpy, win);
		}
		pixmap_copy = XCreatePixmap(dpy, win, src_w, src_h, Pdepth);
		if (pixmap_copy && gc)
		{
			XCopyArea(
				dpy, win, pixmap_copy, gc,
				src_x, src_y, src_w, src_h, 0, 0);
		}
		src_x = src_y = 0;
	}
	else if (tint_percent > 0 && !pixmap_copy)
	{
		if (gc == None)
		{
			gc = PictureDefaultGC(dpy, win);
		}
		pixmap_copy = XCreatePixmap(dpy, win, src_w, src_h, Pdepth);
		if (pixmap_copy && gc)
		{
			XCopyArea(
				dpy, pixmap, pixmap_copy, gc,
				src_x, src_y, src_w, src_h, 0, 0);
		}
		src_x = src_y = 0;
	}
	else if (!pixmap_copy)
	{
		SUPPRESS_UNUSED_VAR_WARNING(pa);
		SUPPRESS_UNUSED_VAR_WARNING(pam);
		src_picture = FRenderCreatePicture(
			dpy, pixmap, PFrenderVisualFormat, pam, &pa);
	}

	if (!src_picture && pixmap_copy)
	{
		src_picture = FRenderCreatePicture(
			dpy, pixmap_copy, PFrenderVisualFormat, pam, &pa);
	}

	if (!src_picture)
	{
		goto bail;
	}

	/* tint the src, it is why we have done a pixmap copy */
	if (tint_percent > 0)
	{
		FRenderTintPicture(
			dpy, win, tint, tint_percent, src_picture,
			src_x, src_y, src_w, src_h);
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
		else
		{
			/* fix a bug in certain XRender server implementation?
			 */
			if (!(shade_picture = FRenderCreateShadePicture(
				dpy, win, 100)))
			{
				goto bail;
			}
			alpha_x = alpha_y = 0;
		}
	}
	else
	{
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
					  alpha_x, alpha_y, src_w, src_h, 0, 0);
				alpha_picture = FRenderCreatePicture(
					dpy, alpha_copy, PFrenderAlphaFormat,
					pam, &pa);
			}
			if (alpha_gc && free_alpha_gc)
			{
				XFreeGC(dpy, alpha_gc);
			}
			alpha_x = alpha_y = 0;
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
		else
		{
			alpha_x = alpha_y = 0;
		}

		if (!(shade_picture = FRenderCreateShadePicture(
			dpy, win, added_alpha_percent)))
		{
			goto bail;
		}


		if (alpha != None && alpha_picture && shade_picture)
		{
			if (!FRenderCompositeAndCheck(
				dpy, FRenderPictOpAtopReverse, shade_picture,
				alpha_picture, alpha_picture,
				0, 0, alpha_x, alpha_y, 0, 0, src_w, src_h))
			{
				goto bail;
			}
			alpha_x = alpha_y = 0;
		}
		else if (mask != None && alpha_picture && shade_picture)
		{
			if (!FRenderCompositeAndCheck(
				dpy, FRenderPictOpAtopReverse, shade_picture,
				mask_picture, alpha_picture,
				0, 0, alpha_x, alpha_y, 0, 0, src_w, src_h))
			{
				goto bail;
			}
			alpha_x = alpha_y = 0;
		}
	}

	if (alpha_picture == None)
	{
		alpha_picture = shade_picture;
	}

	dest_picture = FRenderCreatePicture(
		dpy, d, PFrenderVisualFormat, 0, &pa);

	if (dest_picture)
	{
		rv = FRenderCompositeAndCheck(
			dpy, FRenderPictOpOver, src_picture, alpha_picture,
			dest_picture, src_x, src_y, alpha_x, alpha_y,
			dest_x, dest_y, dest_w, dest_h);
	}

 bail:
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
	if (root_picture)
	{
		FRenderFreePicture(dpy, root_picture);
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
